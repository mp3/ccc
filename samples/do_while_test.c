int main() {
    int i = 0;
    int sum = 0;
    
    // Basic do-while loop that sums 1 to 5
    do {
        i = i + 1;
        sum = sum + i;
    } while (i < 5);
    
    // sum should be 1+2+3+4+5 = 15
    return sum;
}