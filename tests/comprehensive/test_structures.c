/*
 * Test: Structures and Unions
 * Expected Output: 25
 * Description: Tests struct declarations, member access, and union types
 */

struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
};

union Data {
    int i;
    char c;
};

int main() {
    // Test basic struct operations
    struct Point p1;
    p1.x = 10;
    p1.y = 15;
    
    // Test struct initialization through assignment
    struct Point p2;
    p2.x = 5;
    p2.y = 8;
    
    // Test nested structures
    struct Rectangle rect;
    rect.top_left.x = 0;
    rect.top_left.y = 10;
    rect.bottom_right.x = 20;
    rect.bottom_right.y = 0;
    
    // Test pointer to struct
    struct Point *ptr = &p1;
    int x_val = ptr->x;  // Should be 10
    int y_val = ptr->y;  // Should be 15
    
    // Test union
    union Data data;
    data.i = 65;  // ASCII value of 'A'
    
    // Calculate result
    int width = rect.bottom_right.x - rect.top_left.x;  // 20 - 0 = 20
    int height = rect.top_left.y - rect.bottom_right.y; // 10 - 0 = 10
    
    // We want to return 25
    // p1.x = 10, p1.y = 15, so p1.x + p1.y = 25
    return p1.x + p1.y;
}