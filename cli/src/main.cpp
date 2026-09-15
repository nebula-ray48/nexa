#include <iostream>

// 生成されたNexa用Tree-sitterパーサー関数の宣言
extern "C" {
    void* tree_sitter_nexa();
}

int main() {
    std::cout << "Nexa Compiler Core (C++23) initializing..." << std::endl;

    // パーサーが正しくリンクされ、呼び出せるかテスト
    void* parser = tree_sitter_nexa();
    if (parser) {
        std::cout << "[SUCCESS] Tree-sitter Nexa parser loaded perfectly!" << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to load parser." << std::endl;
    }

    return 0;
}
