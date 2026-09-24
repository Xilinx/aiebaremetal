// Mock library without the required symbol
// This library will successfully load with dlopen but dlsym will fail to find the symbol

// Empty library - no createBaremetalGraphInstance function exported
int dummy_function() {
    return 0;
}
