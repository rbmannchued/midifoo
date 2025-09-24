#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char** argv) {
    printf("=== Teste Simples DSP com CMSIS-DSP ===\n");
    
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
