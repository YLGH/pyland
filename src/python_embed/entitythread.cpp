///Quick and dirty hack to get pyland to compile on Ubuntu
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include "python_embed_headers.hpp"

#include <atomic>
#include <boost/filesystem/path.hpp>
#include <boost/python.hpp>
#include <boost/ref.hpp>
#include <future>
#include <glog/logging.h>
#include <glm/vec2.hpp>
#include <thread>

#include "entity.hpp"
#include "entitythread.hpp"
#include "event_manager.hpp"
#include "interpreter_context.hpp"
#include "lifeline.hpp"
#include "locks.hpp"
#include "make_unique.hpp"
#include "game_engine.hpp"

// For PyThread_get_thread_ident
#include "pythread.h"

namespace py = boost::python;

LockableEntityThread::LockableEntityThread():
    lock::Lockable<std::shared_ptr<EntityThread>>() {}

LockableEntityThread::LockableEntityThread(std::shared_ptr<EntityThread> value):
    lock::Lockable<std::shared_ptr<EntityThread>>(value) {}

LockableEntityThread::LockableEntityThread(std::shared_ptr<EntityThread> value, std::shared_ptr<std::mutex> lock):
    lock::Lockable<std::shared_ptr<EntityThread>>(value, lock) {}


//https://stackoverflow.com/questions/1418015/how-to-get-python-exception-text TODO: comment and cleanup this code
std::string handle_pyerror()
{
    using namespace boost::python;
    using namespace boost;

    PyObject *exc,*val,*tb;
    object formatted_list, formatted;
    PyErr_Fetch(&exc,&val,&tb);
    handle<> hexc(exc),hval(allow_null(val)),htb(allow_null(tb)); 
    object traceback(import("traceback"));
    if (!tb) {
        object format_exception_only(traceback.attr("format_exception_only"));
        formatted_list = format_exception_only(hexc,hval);
    } else {
        object format_exception(traceback.attr("format_exception"));
        formatted_list = format_exception(hexc,hval,htb);
    }
    formatted = str("\n").join(formatted_list);
    return extract<std::string>(formatted);
}

///
/// A thread function running a player's daemon.
///
/// @param on_finish
///     Atomic flag to signal the thread's finishing
///
/// @param entity_object
///     Python object to pass to the bootstrapper, this is a list of the objects in the map,
///     which the bootstrapper then presents to the python level code appropriately.
///
/// @param thread_id_promise
///     Promise allowing the thread to asynchronously return the thread's id,
///     according to CPython.
///
/// @param bootstrapper_file
///     The Python file that can bootstrap the process, taking an entity, running
///     it's files and controling logic (such as handling asynchronous exceptions).
///
/// @param interpreter_context
///     The interpreter_context of the main interpreter, allowing creation of a new thread
///     by access of the interpreter's PyInterpreterState.
///
///     Also allows importing files.
/// TODO: Create some kind python engine object and suitable api for engine stuff!!!!!
void run_entities(std::atomic<bool> &on_finish,
                std::shared_ptr<py::api::object> entities_object,
                py::api::object game_engine_object,
                std::promise<long> thread_id_promise,
                boost::filesystem::path bootstrapper_file,
                InterpreterContext interpreter_context,
                std::map<EntityThread::Signal, PyObject *> signal_to_exception) {

    LOG(INFO) << "run_entity: Starting";
    Lifeline alert_on_finish([&] () { on_finish = true; });

    bool waiting = true;

    // Register thread with Python, to allow locking
    lock::ThreadState threadstate(interpreter_context);

    std::unique_ptr<py::api::object> bootstrapper_module;

    {
        lock::ThreadGIL lock_thread(threadstate);

        LOG(INFO) << "run_entity: Stolen GIL";
        
        try {
            // Import the bootstrapper file, check it for errors
            bootstrapper_module = std::make_unique<py::api::object>(
                interpreter_context.import_file(bootstrapper_file)
            );
        } catch (py::error_already_set &) { //catch any errors in the setup.
            std::string msg = handle_pyerror();
            LOG(INFO) << msg;
            throw std::runtime_error("Python error");
        }

        // Asynchronously return thread id to allow killing of this thread
        //
        // WARNING:
        //     This causes subtle race conditions, as setting this requires
        //     the GIL and the thread killer takes the GIL.
        //
        //     BE CAREFUL.
        //
        thread_id_promise.set_value(PyThread_get_thread_ident());
    }

    while (true) {
        try {
            lock::ThreadGIL lock_thread(threadstate);
            bootstrapper_module->attr("start")(
                *entities_object,
                game_engine_object,
                py::api::object(py::borrowed<>(signal_to_exception[EntityThread::Signal::RESTART])),
                py::api::object(py::borrowed<>(signal_to_exception[EntityThread::Signal::STOP])),
                py::api::object(py::borrowed<>(signal_to_exception[EntityThread::Signal::KILL])),
                waiting
            );
        }
        catch (py::error_already_set &) {

            lock::ThreadGIL lock_thread(threadstate);

            PyObject *type, *value, *traceback;
            PyErr_Fetch(&type, &value, &traceback);

            if (!type) {
                throw std::runtime_error("Unknown Python error");
            }

            if (PyErr_GivenExceptionMatches(signal_to_exception[EntityThread::Signal::RESTART], type)) {
                waiting = false;
                continue;
            }
            else if (PyErr_GivenExceptionMatches(signal_to_exception[EntityThread::Signal::STOP], type)) {
                // Just wait.
                waiting = true;
                continue;
            }
            else if (PyErr_GivenExceptionMatches(signal_to_exception[EntityThread::Signal::KILL], type)) {
                // We are done.
                LOG(INFO) << "Thread is killed";
                return;
            }
            else {
                py::handle<> hType(type);
                py::object extype(hType);
                py::handle<> hTraceback(traceback);
                py::object ptraceback(hTraceback);
                LOG(ERROR) << "Python error, details: ";
                std::string error_message = py::extract<std::string>(value);
                LOG(ERROR) << error_message; //Log the error message.
            }
        }
        waiting = true;
    }

    LOG(INFO) << "run_entity: Finished";
}

//TODO: This was based on a version which created a thread for each entity in the level, now it takes a list of entities and creates a thread for them,
//This needs to be renamed or maybe refactored appropriately
EntityThread::EntityThread(InterpreterContext interpreter_context, std::list<Entity> &entities, GameEngine &game_engine):
    entities(entities),
    previous_call_number(entities.front().call_number), //TODO: Work out what this did!!!!!!
    interpreter_context(interpreter_context),

    thread_finished(false),

    Py_BaseAsyncException(make_base_async_exception(PyExc_BaseException, "__main__.BaseAsyncException")),

    signal_to_exception({
        {
            EntityThread::Signal::RESTART,
            make_base_async_exception(Py_BaseAsyncException, "__main__.BaseAsyncException_RESTART")
        }, {
            EntityThread::Signal::STOP,
            make_base_async_exception(Py_BaseAsyncException, "__main__.BaseAsyncException_STOP")
        }, {
            EntityThread::Signal::KILL,
            make_base_async_exception(Py_BaseAsyncException, "__main__.BaseAsyncException_KILL")
        }
    })

    {
        // To get thread_id
        std::promise<long> thread_id_promise;
        thread_id_future = thread_id_promise.get_future();

        
        entity_object = std::make_shared<py::list>(); //A python list of all the game_objects is what we want to pass to the boostrapper, so it can handle and name them

        //Go through this list of entities, wrap them using boost and then append them to the python list of objects :)
        for(auto &entity: entities) {
            // Wrap the object for Python.
            //
            // For implementation justifications, see
            // http://stackoverflow.com/questions/24477791
            {
                lock::GIL lock_gil(interpreter_context, "EntityThread::EntityThread");
                entity_object->append(boost::ref(entity));
            };
        }

        py::api::object game_engine_object = py::api::object(boost::ref(game_engine));

        thread = std::thread(
            run_entities,
            std::ref(thread_finished),
            entity_object,
            game_engine_object,
            std::move(thread_id_promise),
            // TODO: Extract path into a more logical place
            boost::filesystem::path("../game/bootstrapper.py"), //TODO: Move this configuration out to an ini file!
            interpreter_context,
            signal_to_exception
        );
}

long EntityThread::get_thread_id() {
    if (thread_id_future.valid()) {
        thread_id = thread_id_future.get();
    }

    return thread_id;
}


PyObject *EntityThread::make_base_async_exception(PyObject *base, const char *name) {
    lock::GIL lock_gil(interpreter_context, "EntityThread::make_base_async_exception");

    return PyErr_NewException(name, base, nullptr);
}

void EntityThread::halt_soft(Signal signal) {
    auto thread_id = get_thread_id();

    lock::GIL lock_gil(interpreter_context, "EntityThread::halt_soft");

    PyThreadState_SetAsyncExc(thread_id, signal_to_exception[signal]);
}

void EntityThread::halt_hard() {
    // TODO: everything!!!
    throw std::runtime_error("hard halting not implemented");

    lock::GIL lock_gil(interpreter_context, "EntityThread::halt_hard");
}

bool EntityThread::is_dirty() {
    return previous_call_number != entities.front().call_number; //TODO: Work out what this did!!!!!!
}

void EntityThread::clean() {
    previous_call_number = entities.front().call_number;//TODO: Work out what this did!!!!!!
}

void EntityThread::finish() {
    // TODO: implement nagging
    while (true) {
        halt_soft(Signal::KILL);

        if (thread_finished) { break; }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    thread.join();
}

EntityThread::~EntityThread() {
    finish();
    LOG(INFO) << "EntityThread destroyed";

    lock::GIL lock_gil(interpreter_context, "EntityThread::~EntityThread");

    if (!entity_object.unique()) {
        throw std::runtime_error("multiple references to entity_object on destruction"); //TODO: WORK OUT WHAT THIS DID!
    }

    for (auto signal_exception : signal_to_exception) {
        Py_DECREF(signal_exception.second);
    }
    Py_DECREF(Py_BaseAsyncException);

    entity_object.reset();
}
