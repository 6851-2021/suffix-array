sa: sa.cpp
	$(CXX) sa.cpp -o sa -O3 $(CXXFLAGS)

sa-debug: sa.cpp
	$(CXX) sa.cpp -o sa-debug -fsanitize=address $(CXXFLAGS)

clean:
	rm sa sa-debug -f

.PHONY: clean
