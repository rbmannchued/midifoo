#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char** argv) {
    printf("=== DSP Tests Using Samples ===\n");
    
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
