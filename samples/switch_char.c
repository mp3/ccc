int main() {
    char grade = 'B';
    int score = 0;
    
    switch (grade) {
        case 'A':
            score = 90;
            break;
        case 'B':
            score = 80;
            break;
        case 'C':
            score = 70;
            break;
        case 'D':
            score = 60;
            break;
        default:
            score = 0;
            break;
    }
    
    // grade is 'B', so score should be 80
    return score;
}