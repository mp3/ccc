// Test floating point support

int main() {
    float pi = 3.14159f;
    double e = 2.71828;
    
    float radius = 5.0f;
    float area = pi * radius * radius;
    
    double x = 1.5;
    double y = 2.5;
    double sum = x + y;
    
    // Test various float literals
    float f1 = 123.456f;
    float f2 = 1.23e2f;   // Scientific notation
    float f3 = .5f;       // Leading decimal
    double d1 = 3.14159265359;
    double d2 = 6.022e23; // Avogadro's number
    
    return 0;
}
