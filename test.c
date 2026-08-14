int main() {
    int x = 10;
    int count123 = 42;
    float price = 19.99;
    char letter = 'A';

    x += 5;
    x -= 2;
    x *= 3;
    x /= 2;
    x %= 4;

    if (x == 10) {
        x = x + 1;
    } else {
        x = x - 1;
    }

    while (x < 100) {
        x = x + 10;
    }

    if (x >= 50 && x != 75) {
        x = x * 2;
    }

    if (!x) {
        x = 0;
    }

    return x;
}