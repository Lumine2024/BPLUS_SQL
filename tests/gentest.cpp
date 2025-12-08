#include <random>
#include <iostream>
#include <string>

char buffer[1 << 24];

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.rdbuf()->pubsetbuf(buffer, 1 << 24);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> opdist(0, 2);
    std::uniform_int_distribution<int> keydist(1, 100000);
    const std::string DB_NAME = "test_db";

    std::cout << "CREATE TABLE " << DB_NAME << '\n';

    for (int i = 0; i < 100000; ++i) {
        int op = opdist(rng);
        int k = keydist(rng);
        if (op == 0) {
            std::cout << "INSERT INTO " << DB_NAME << " KEY " << k << '\n';
        } else if (op == 1) {
            std::cout << "ERASE FROM " << DB_NAME << " KEY " << k << '\n';
        } else {
            std::cout << "QUERY FROM " << DB_NAME << " KEY " << k << '\n';
        }
    }

    std::cout << "DESTROY TABLE " << DB_NAME << '\n';
    return 0;
}
