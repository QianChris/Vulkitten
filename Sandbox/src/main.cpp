namespace Vulkitten
{
    _declspec(dllimport) void Print();
} 

int main() {
    Vulkitten::Print();
    return 0;
}
