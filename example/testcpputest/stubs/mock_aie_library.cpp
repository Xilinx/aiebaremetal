#include <cstdint>
#include <string>

// Mock implementation of BaremetalGraphSimBase for the mock library
class MockLibraryGraph {
public:
    MockLibraryGraph(const char* name, uint8_t rpu) {}
    virtual ~MockLibraryGraph() {}
};

// Export the createBaremetalGraphInstance function that dlsym will look for
extern "C" {
    void* createBaremetalGraphInstance(const char* name, const uint8_t rpu) {
        return new MockLibraryGraph(name, rpu);
    }
}
