#ifndef TASKINTERFACE_HPP
#define TASKINTERFACE_HPP

// Clase base abstracta que define la interfaz de una tarea
// Cualquier cosa que pueda ser ejecutada por el ThreadPool debe heredar de Task
class Task {
   public:
    // Destructor virtual (importante en clases base con herencia)
    virtual ~Task() = default;

    // Método puro virtual que DEBE implementar cada clase derivada
    // Esto es lo que el ThreadPool ejecutará
    virtual void execute() = 0;
};

#endif
