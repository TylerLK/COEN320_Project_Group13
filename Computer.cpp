#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

<<<<<<< Updated upstream:Computer.cpp
int main()
{
    cout << "This is the Computer Subsystem" << endl;
=======
void printGrid(const vector<pair<int, int>>& points, int width, int height) {
    vector<vector<char>> grid(height, vector<char>(width, '.'));

    for (const auto& point : points) {
        if (point.first >= 0 && point.first < width && point.second >= 0 && point.second < height) {
            grid[point.second][point.first] = 'X';
        }
    }

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            cout << grid[y][x] << ' ';
        }
        cout << endl;
    }
}

int main() {
    cout << "Welcome to our COEN320 project" << endl;

    int width = 10;
    int height = 10;
    vector<pair<int, int>> points = {{1, 1}, {2, 4}, {3, 9}, {4, 16}, {5, 25}};

    for (int i = 0; i < 10; ++i) {
        system("cls"); // Clear the console (use "clear" for Linux/Mac)
        printGrid(points, width, height);
        this_thread::sleep_for(chrono::seconds(1));

        // Update data points
        for (auto& point : points) {
            point.second = (point.second + 1) % height;
        }
    }

>>>>>>> Stashed changes:main.cpp
    return 0;
}