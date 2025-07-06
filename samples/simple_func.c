int square(int x) {
    return x * x;
}

int main() {
    int a = 5;
    int b = square(a);
    return b;  // Should return 25
}