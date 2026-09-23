//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_OVERLOADED_H
#define PROYECTO1PG3_OVERLOADED_H


#pragma once
// Clase variádica Overloaded (enunciado §6.4): combina varias lambdas en un
// solo objeto invocable para usarlo con std::visit.

namespace circuit_escape {

    template <typename... Callables>
    struct Overloaded : Callables... {
        using Callables::operator()...;   // expansión del paquete en la using-declaration
    };

    template <typename... Callables>
    Overloaded(Callables...) -> Overloaded<Callables...>;   // guía de deducción

}

#endif //PROYECTO1PG3_OVERLOADED_H