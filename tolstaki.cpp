#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <locale>
#include <windows.h>

using namespace std;

mutex mtx;
bool cook_can_work = true;
bool fatmen_can_eat = false;
int ready_dishes = 0;

int dish1, dish2, dish3;
int fatman1_eaten, fatman2_eaten, fatman3_eaten;
bool simulation_running;
bool scenario_completed;

void reset_simulation() {
    dish1 = dish2 = dish3 = 3000;
    fatman1_eaten = fatman2_eaten = fatman3_eaten = 0;
    simulation_running = true;
    scenario_completed = false;
    cook_can_work = true;
    fatmen_can_eat = false;
    ready_dishes = 0;
}

void cook(int efficiency_factor) {
    while (simulation_running && !scenario_completed) {
        while (!cook_can_work && simulation_running && !scenario_completed) {
            this_thread::yield();
        }

        if (!simulation_running || scenario_completed) break;

        mtx.lock();

        dish1 += efficiency_factor;
        dish2 += efficiency_factor;
        dish3 += efficiency_factor;

        ready_dishes = 3;
        cook_can_work = false;
        fatmen_can_eat = true;

        mtx.unlock();
    }
}

void fatman(int fatman_id, int gluttony) {
    int& dish = (fatman_id == 1) ? dish1 : (fatman_id == 2) ? dish2 : dish3;
    int& eaten = (fatman_id == 1) ? fatman1_eaten : (fatman_id == 2) ? fatman2_eaten : fatman3_eaten;

    while (simulation_running && !scenario_completed) {
        while (!fatmen_can_eat && simulation_running && !scenario_completed) {
            this_thread::yield();
        }

        if (!simulation_running || scenario_completed) break;

        mtx.lock();

        // Проверяем условия завершения сценария
        bool any_empty = (dish1 <= 0) || (dish2 <= 0) || (dish3 <= 0);
        bool all_burst = (fatman1_eaten >= 10000) && (fatman2_eaten >= 10000) && (fatman3_eaten >= 10000);

        if (any_empty || all_burst) {
            scenario_completed = true;
            mtx.unlock();
            break;
        }

        if (ready_dishes > 0 && dish >= gluttony) {
            dish -= gluttony;
            eaten += gluttony;
            ready_dishes--;

            if (ready_dishes == 0) {
                fatmen_can_eat = false;
                cook_can_work = true;
            }
        }

        mtx.unlock();
    }
}

void run_scenario(int efficiency_factor, int gluttony, const string& scenario_name, int expected_scenario) {
    reset_simulation();

    cout << "\n=== " << scenario_name << " ===" << endl;
    cout << "Коэффициенты: efficiency=" << efficiency_factor << ", gluttony=" << gluttony << endl;

    thread cook_thread(cook, efficiency_factor);
    thread fatman1(fatman, 1, gluttony);
    thread fatman2(fatman, 2, gluttony);
    thread fatman3(fatman, 3, gluttony);

    auto start = chrono::steady_clock::now();
    int elapsed_seconds = 0;

    while (simulation_running && !scenario_completed && elapsed_seconds < 5) {
        auto now = chrono::steady_clock::now();
        elapsed_seconds = chrono::duration_cast<chrono::seconds>(now - start).count();
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    simulation_running = false;
    scenario_completed = true;

    cook_thread.join();
    fatman1.join();
    fatman2.join();
    fatman3.join();

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end - start);

    cout << "Результаты после " << duration.count() << " секунд:" << endl;
    cout << "- Съедено толстяками: " << fatman1_eaten << ", " << fatman2_eaten << ", " << fatman3_eaten << endl;
    cout << "- Осталось в тарелках: " << dish1 << ", " << dish2 << ", " << dish3 << endl;

    bool any_empty = (dish1 <= 0) || (dish2 <= 0) || (dish3 <= 0);
    bool all_burst = (fatman1_eaten >= 10000) && (fatman2_eaten >= 10000) && (fatman3_eaten >= 10000);
    bool time_out = (duration.count() >= 5);

    // Проверяем соответствие ожидаемому сценарию
    bool success = false;

    if (expected_scenario == 1) { // Кука уволили
        success = any_empty && !time_out;
        cout << "ВЫВОД: " << (success ? "✓ Кука уволили! (тарелка пустая ДО 5 дней)" : "✗ Не достигнуто") << endl;
    }
    else if (expected_scenario == 2) { // Не получил зарплату
        success = all_burst && !time_out;
        cout << "ВЫВОД: " << (success ? "✓ Кук не получил зарплату! (все лопнули ДО 5 дней)" : "✗ Не достигнуто") << endl;
    }
    else if (expected_scenario == 3) { // Уволился сам
        success = time_out && !any_empty && !all_burst;
        cout << "ВЫВОД: " << (success ? "✓ Кук уволился сам! (прошло 5 дней, все в норме)" : "✗ Не достигнуто") << endl;
    }

    if (!success) {
        cout << "   (Причина: ";
        if (time_out) cout << "время вышло, ";
        if (any_empty) cout << "тарелка пустая, ";
        if (all_burst) cout << "все лопнули";
        cout << ")" << endl;
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    cout << "ЛАБОРАТОРНАЯ РАБОТА №4: ТРИ ТОЛСТЯКА" << endl;
    cout << "Демонстрация трех сценариев..." << endl;

    // Сценарий 1: Кука уволили - тарелка пустеет БЫСТРО (до 5 дней)
    // Толстяки едят намного быстрее чем повар успевает готовить
    run_scenario(5, 150, "СЦЕНАРИЙ 1: Кука уволили", 1);

    // Сценарий 2: Кук не получил зарплату - все толстяки лопаются БЫСТРО (до 5 дней)
    // Повар готовит очень много, толстяки быстро набирают 10000+
    run_scenario(300, 250, "СЦЕНАРИЙ 2: Кук не получил зарплату", 2);

    // Сценарий 3: Кук уволился сам - за 5 дней ничего критичного не происходит
    // Сбалансированные коэффициенты
    run_scenario(20, 15, "СЦЕНАРИЙ 3: Кук уволился сам", 3);

    cout << "\nПрограмма завершена." << endl;
    return 0;
}