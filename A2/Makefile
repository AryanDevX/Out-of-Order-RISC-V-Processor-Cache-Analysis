CXX = g++
CXXFLAGS = -std=c++17 -Wall
 
compile:
	$(CXX) $(CXXFLAGS) Processor.cpp ExecutionUnit.cpp LoadStoreQueue.cpp BranchPredictor.cpp $(FILE) -o main
 
run:
	$(CXX) $(CXXFLAGS) Preprocess.cpp -o preprocess
	./Preprocess $(FILE)