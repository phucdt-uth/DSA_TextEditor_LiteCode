// Diem khoi chay chinh cua chuong trinh soan thao van ban
#include <iostream>
#include <string>
#include "Editor.h"
#include "UI.h"

using namespace std;

int main(int argc, char** argv) {
    Editor ed;
    string err;

    if (argc > 1) {
        if (!ed.openFile(argv[1], err)) {
            ed.setFileName(argv[1]);
            cout << "[Thong bao] Tao tep moi: " << argv[1] << "\n";
        }
    }
    else if (ed.openFile("sample_30.txt", err)) {
        cout << "[Thong bao] Da nap du lieu mau: sample_30.txt\n";
    }

    UI ui;
    ui.run(ed);
    return 0;
}