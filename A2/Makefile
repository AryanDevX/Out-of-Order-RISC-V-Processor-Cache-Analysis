CXX = g++
CXXFLAGS = -std=c++17 -Wall

compile:
	$(CXX) $(CXXFLAGS) Processor.cpp ExecutionUnit.cpp LoadStoreQueue.cpp BranchPredictor.cpp $(FILE) -o main

run:
	@echo "Preprocessing $(FILE)... (handled at runtime)"