#ifndef CIRCUIT_ESCAPE_TEST_CHECK_HPP
#define CIRCUIT_ESCAPE_TEST_CHECK_HPP

// Verificación simple para las pruebas.
// A diferencia de assert, funciona también en modo Release y no detiene el programa:
// cuenta los fallos y al final la prueba devuelve 1 si hubo alguno.

#include <iostream>
#include <string>

inline int g_failures = 0;

inline void check(bool condition, const std::string& description) {
    if (!condition) {
        std::cerr << "FALLO: " << description << '\n';
        ++g_failures;
    }
}

inline int test_exit_code() {
    if (g_failures == 0) {
        std::cout << "Todas las pruebas pasaron.\n";
        return 0;
    }
    std::cerr << g_failures << " prueba(s) fallaron.\n";
    return 1;
}

#endif  // CIRCUIT_ESCAPE_TEST_CHECK_HPP
