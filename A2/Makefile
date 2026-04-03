# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# ==========================================
# make compile FILE=<filename.cpp>
# ==========================================
# This target compiles your main file along with all other 
# component files (Processor, ExecutionUnit, etc.) into one binary.
compile:
	@echo "Compiling simulator:"
	$(CXX) $(CXXFLAGS) *.cpp -o main
	@echo "Build successful, 'main' created."

# ==========================================
# make run FILE=<filename.s>
# ==========================================
# Since the assembly preprocessor is built directly into our 
# C++ loadProgram function, we just need to execute the compiled 
# binary and pass the .s file to it!
run:
	@echo "Loading and executing $(FILE)..."
	./main $(FILE)