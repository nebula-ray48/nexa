#include <iostream>

namespace nexa {
void parse_test_code();
}

int main() {
    std::cout << "--- Nexa Compiler Core ---" << std::endl;

    // パース処理の実行
    nexa::parse_test_code();

    return 0;
}