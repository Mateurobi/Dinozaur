#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <algorithm>

using namespace std;

const int SZEROKOSC_MAPY = 20;
const int CZAS_SKOKU = 6;
const int MIN_ODSTEP_X = 8;
const int MAX_ODSTEP_X = 12;

struct termios orig_termios;

class Obiekt {
protected:
    string nazwa;
    int x, y;
    char symbol;

public:
    Obiekt(string n, int sx, int sy, char s)
        : nazwa(n), x(sx), y(sy), symbol(s) {}
    void przesunWLewo() { x--; }
    int getX() const { return x; }
    int getY() const { return y; }
    char getSymbol() const { return symbol; }
    void setY(int ny) { y = ny; }
};

class Dinozaur : public Obiekt {
public:
    Dinozaur() : Obiekt("Dino", 2, 0, 'D') {}
};

class Kaktus : public Obiekt {
    int rozmiar;

public:
    Kaktus(int x_pos) : Obiekt("Kaktus", x_pos, 0, '#') {
        int los = rand() % 100;
        if (los < 70) rozmiar = 1;
        else if (los < 95) rozmiar = 2;
        else rozmiar = 3;
    }
    int getRozmiar() const { return rozmiar; }
};

class Pterodaktyl : public Obiekt {
public:
    Pterodaktyl(int x_pos) : Obiekt("Ptero", x_pos, 1, 'W') {}
};

void ustawTerminal() {
    tcgetattr(0, &orig_termios);
    termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &raw);
    fcntl(0, F_SETFL, O_NONBLOCK);
}

void resetTerminal() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

double obliczMnoznikPredkosci(int punkty) {
    if (punkty < 100) return 1.0;
    else if (punkty < 250) return 1.5;
    else if (punkty < 350) return 2.0;
    else if (punkty < 500) return 2.5;
    else return 3.0;
}

int obliczOpoznienie(int punkty) {
    double mnoznik = obliczMnoznikPredkosci(punkty);
    return static_cast<int>(300000 / mnoznik);
}

bool sprawdzanieKolizji(const Dinozaur& d, const vector<Kaktus>& k, const vector<Pterodaktyl>& p) {
    for (auto &x : k)
        for (int i = 0; i < x.getRozmiar(); i++)
            if (d.getX() == x.getX() + i && d.getY() == 0)
                return true;
    for (auto &x : p)
        if (d.getX() == x.getX() && d.getY() == 1)
            return true;
    return false;
}

int main() {
    srand(time(0));
    ustawTerminal();
    int najlepszyWynik = 0;

    while (true) {
        Dinozaur gracz;
        vector<Kaktus> kaktusy;
        vector<Pterodaktyl> ptero;
        int skok = 0;
        int punkty = 0;
        int aktualnyOdstęp = MIN_ODSTEP_X + rand() % (MAX_ODSTEP_X - MIN_ODSTEP_X + 1);

        while (true) {
            system("clear");
            string gora(SZEROKOSC_MAPY, '-');
            string dol(SZEROKOSC_MAPY, '_');

            if (gracz.getY() == 1) gora[gracz.getX()] = 'D';
            else dol[gracz.getX()] = 'D';

            for (auto &k : kaktusy)
                for (int i = 0; i < k.getRozmiar(); i++)
                    if (k.getX() + i >= 0 && k.getX() + i < SZEROKOSC_MAPY)
                        dol[k.getX() + i] = '#';

            for (auto &p : ptero)
                if (p.getX() >= 0 && p.getX() < SZEROKOSC_MAPY)
                    gora[p.getX()] = 'W';

            cout << "\n" << gora << "\n" << dol;
            cout << "\nPunkty: " << punkty
                 << "   Szybkosc: X" << obliczMnoznikPredkosci(punkty) << endl;
            cout << "Nacisnij 's' lub SPACJA aby skoczyc (q = wyjscie)" << endl;

            char c;
            while (read(0, &c, 1) == 1) {
                if ((c == 's' || c == ' ') && skok == 0) skok = CZAS_SKOKU;
                if (c == 'q') {
                    resetTerminal();
                    return 0;
                }
            }

            if (skok > 0) {
                gracz.setY(1);
                skok--;
                if (skok == 0) gracz.setY(0);
            }

            for (auto &k : kaktusy) k.przesunWLewo();
            for (auto &p : ptero) p.przesunWLewo();

            kaktusy.erase(remove_if(kaktusy.begin(), kaktusy.end(),
                [](Kaktus &k){ return k.getX() < -5; }), kaktusy.end());

            ptero.erase(remove_if(ptero.begin(), ptero.end(),
                [](Pterodaktyl &p){ return p.getX() < -5; }), ptero.end());

            int najblizszaX = -100;
            for (auto &k : kaktusy) if (k.getX() > najblizszaX) najblizszaX = k.getX();
            for (auto &p : ptero) if (p.getX() > najblizszaX) najblizszaX = p.getX();

            if ((kaktusy.empty() && ptero.empty()) || 19 - najblizszaX >= aktualnyOdstęp) {
                if (rand() % 100 < 85) kaktusy.emplace_back(19);
                else ptero.emplace_back(19);
                aktualnyOdstęp = MIN_ODSTEP_X + rand() % (MAX_ODSTEP_X - MIN_ODSTEP_X + 1);
            }

            if (sprawdzanieKolizji(gracz, kaktusy, ptero)) break;

            punkty++;
            usleep(obliczOpoznienie(punkty));
        }

        if (punkty > najlepszyWynik) najlepszyWynik = punkty;

        cout << "\nKONIEC GRY! Punkty: " << punkty << "   Najlepszy wynik: " << najlepszyWynik << endl;
        cout << "Wcisnij 'r' aby zagrac ponownie lub 'q' aby wyjsc." << endl;

        char wybor;
        while (true) {
            while (read(0, &wybor, 1) != 1) usleep(10000);
            if (wybor == 'r') break;
            if (wybor == 'q') {
                resetTerminal();
                return 0;
            }
        }
    }
}



