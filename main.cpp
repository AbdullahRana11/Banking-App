#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <direct.h>

using namespace std;
using namespace sf;

// Abdullah: setting up the basic data structures here. Huzaifa, make sure your file parser matches this exact order!

struct Account {
    int accNumber;
    string holderName;
    string cnic;
    string phone;
    string address;
    string accType;         // "Savings" or "Current"
    double balance;
    int pin;                // Stored encoded
    bool isActive;
    int failedAttempts;
    double dailyWithdrawn;
    string lastWithdrawDate;
    bool hasLoan;
    double loanAmount;
    double loanPaid;
    double interestRate;
    int loanMonths;
    string loanStartDate;
};

struct Transaction {
    int transId;
    int accNumber;
    string type;            // "Deposit", "Withdrawal", "Transfer-Out", "Transfer-In", etc.
    double amount;
    string dateTime;
    double balanceAfter;
    int targetAccNumber;    // -1 if N/A
};

struct ATMCash {
    int notes5000;
    int notes1000;
    int notes500;
    int notes100;
};

struct AuditEntry {
    int logId;
    string action;
    string dateTime;
    int accNumber;
    string performedBy;
    string details;
};

// --- Security and helper funcs (Abdullah's part) ---

const int PIN_XOR_KEY = 5839; // Abdullah: DO NOT change this key or old accounts will break and passwords will fail
const double DAILY_WITHDRAW_LIMIT = 50000.0;
const double OTP_THRESHOLD = 50000.0;
const int MAX_PIN_ATTEMPTS = 3;
const double SAVINGS_INTEREST_RATE = 7.0;

int encodePin(int pin) { return pin ^ PIN_XOR_KEY; }
int decodePin(int encoded) { return encoded ^ PIN_XOR_KEY; }

string getCurrentDateTime() {
    time_t now = time(0);
    struct tm timeInfo;
    localtime_s(&timeInfo, &now);
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", &timeInfo);
    return string(buffer);
}

string getCurrentDate() {
    time_t now = time(0);
    struct tm timeInfo;
    localtime_s(&timeInfo, &now);
    char buffer[15];
    strftime(buffer, 15, "%Y-%m-%d", &timeInfo);
    return string(buffer);
}

int generateOTP() {
    srand((unsigned int)time(0));
    return 1000 + rand() % 9000;
}

string formatMoney(double amount) {
    stringstream ss;
    ss << fixed << setprecision(2) << amount;
    string result = ss.str();
    int dotPos = result.find('.');
    string intPart = result.substr(0, dotPos);
    string decPart = result.substr(dotPos);
    
    string formatted = "";
    int count = 0;
    for (int i = intPart.length() - 1; i >= 0; i--) {
        if (count == 3 && i > 0) {
            formatted = "," + formatted;
            count = 0;
        }
        formatted = intPart[i] + formatted;
        count++;
    }
    return "Rs. " + formatted + decPart;
}

double toDouble(const string& str) {
    if (str.empty()) return 0.0;
    stringstream ss(str);
    double val = 0.0;
    ss >> val;
    return val;
}

int toInt(const string& str) {
    if (str.empty()) return 0;
    stringstream ss(str);
    int val = 0;
    ss >> val;
    return val;
}

void ensureDirectory(const string& path) {
    _mkdir(path.c_str());
}

// --- Huzaifa's File Handling Code ---
// Huzaifa: making sure all files save properly so we don't lose data when the app closes
const char DELIM = '|';

// Forward declaration of audit log to use in account logic
void saveAuditEntry(const string& action, int accNumber, const string& performer, const string& details);

void writeAccountLine(ofstream& out, const Account& acc) {
    out << acc.accNumber << DELIM << acc.holderName << DELIM << acc.cnic << DELIM
        << acc.phone << DELIM << acc.address << DELIM << acc.accType << DELIM
        << fixed << setprecision(2) << acc.balance << DELIM << acc.pin << DELIM
        << (acc.isActive ? 1 : 0) << DELIM << acc.failedAttempts << DELIM
        << fixed << setprecision(2) << acc.dailyWithdrawn << DELIM << acc.lastWithdrawDate << DELIM
        << (acc.hasLoan ? 1 : 0) << DELIM << fixed << setprecision(2) << acc.loanAmount << DELIM
        << fixed << setprecision(2) << acc.loanPaid << DELIM << fixed << setprecision(2) << acc.interestRate << DELIM
        << acc.loanMonths << DELIM << acc.loanStartDate << "\n";
}

Account parseAccountLine(const string& line) {
    Account acc;
    stringstream ss(line);
    string token;
    getline(ss, token, DELIM); acc.accNumber = toInt(token);
    getline(ss, acc.holderName, DELIM);
    getline(ss, acc.cnic, DELIM);
    getline(ss, acc.phone, DELIM);
    getline(ss, acc.address, DELIM);
    getline(ss, acc.accType, DELIM);
    getline(ss, token, DELIM); acc.balance = toDouble(token);
    getline(ss, token, DELIM); acc.pin = toInt(token);
    getline(ss, token, DELIM); acc.isActive = (token == "1");
    getline(ss, token, DELIM); acc.failedAttempts = toInt(token);
    getline(ss, token, DELIM); acc.dailyWithdrawn = toDouble(token);
    getline(ss, acc.lastWithdrawDate, DELIM);
    getline(ss, token, DELIM); acc.hasLoan = (token == "1");
    getline(ss, token, DELIM); acc.loanAmount = toDouble(token);
    getline(ss, token, DELIM); acc.loanPaid = toDouble(token);
    getline(ss, token, DELIM); acc.interestRate = toDouble(token);
    getline(ss, token, DELIM); acc.loanMonths = toInt(token);
    getline(ss, acc.loanStartDate, DELIM);
    return acc;
}

vector<Account> loadAccounts() {
    vector<Account> accounts;
    ifstream file("data/accounts.txt");
    if (!file.is_open()) return accounts;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        accounts.push_back(parseAccountLine(line));
    }
    file.close();
    return accounts;
}

void saveAllAccounts(const vector<Account>& accounts) {
    ensureDirectory("data");
    ofstream file("data/accounts.txt");
    for (size_t i = 0; i < accounts.size(); i++) {
        writeAccountLine(file, accounts[i]);
    }
    file.close();
}

int getNextAccountNumber() {
    vector<Account> accounts = loadAccounts();
    int maxNum = 1000;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber > maxNum) maxNum = accounts[i].accNumber;
    }
    return maxNum + 1;
}

Transaction parseTransactionLine(const string& line) {
    Transaction t;
    stringstream ss(line);
    string token;
    getline(ss, token, DELIM); t.transId = toInt(token);
    getline(ss, token, DELIM); t.accNumber = toInt(token);
    getline(ss, t.type, DELIM);
    getline(ss, token, DELIM); t.amount = toDouble(token);
    getline(ss, t.dateTime, DELIM);
    getline(ss, token, DELIM); t.balanceAfter = toDouble(token);
    getline(ss, token, DELIM); t.targetAccNumber = toInt(token);
    return t;
}

vector<Transaction> loadTransactions() {
    vector<Transaction> trans;
    ifstream file("data/transactions.txt");
    if (!file.is_open()) return trans;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        trans.push_back(parseTransactionLine(line));
    }
    file.close();
    return trans;
}

void saveTransaction(const Transaction& t) {
    ensureDirectory("data");
    ofstream file("data/transactions.txt", ios::app);
    file << t.transId << DELIM << t.accNumber << DELIM << t.type << DELIM
        << fixed << setprecision(2) << t.amount << DELIM << t.dateTime << DELIM
        << fixed << setprecision(2) << t.balanceAfter << DELIM << t.targetAccNumber << "\n";
    file.close();
}

int getNextTransactionId() {
    vector<Transaction> trans = loadTransactions();
    int maxId = 10000;
    for (size_t i = 0; i < trans.size(); i++) {
        if (trans[i].transId > maxId) maxId = trans[i].transId;
    }
    return maxId + 1;
}

vector<AuditEntry> loadAuditLog() {
    vector<AuditEntry> entries;
    ifstream file("data/audit.txt");
    if (!file.is_open()) return entries;
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        AuditEntry e;
        stringstream ss(line);
        string token;
        getline(ss, token, DELIM); e.logId = toInt(token);
        getline(ss, e.action, DELIM);
        getline(ss, e.dateTime, DELIM);
        getline(ss, token, DELIM); e.accNumber = toInt(token);
        getline(ss, e.performedBy, DELIM);
        getline(ss, e.details, DELIM);
        entries.push_back(e);
    }
    file.close();
    return entries;
}

void saveAuditEntry(const string& action, int accNumber, const string& performer, const string& details) {
    ensureDirectory("data");
    vector<AuditEntry> entries = loadAuditLog();
    int nextId = 1;
    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].logId >= nextId) nextId = entries[i].logId + 1;
    }
    ofstream file("data/audit.txt", ios::app);
    file << nextId << DELIM << action << DELIM << getCurrentDateTime() << DELIM
        << accNumber << DELIM << performer << DELIM << details << "\n";
    file.close();
}

ATMCash loadInventory() {
    ATMCash cash = { 100, 200, 300, 500 }; // defaults
    ifstream file("data/inventory.txt");
    if (!file.is_open()) return cash;
    file >> cash.notes5000 >> cash.notes1000 >> cash.notes500 >> cash.notes100;
    file.close();
    return cash;
}

void saveInventory(const ATMCash& cash) {
    ensureDirectory("data");
    ofstream file("data/inventory.txt");
    file << cash.notes5000 << " " << cash.notes1000 << " " << cash.notes500 << " " << cash.notes100 << endl;
    file.close();
}

void generateReceipt(const Transaction& t, const string& holderName) {
    ensureDirectory("data");
    ensureDirectory("data/receipts");
    string filename = "data/receipts/receipt_" + to_string(t.transId) + ".txt";
    ofstream file(filename);
    file << "========================================\n";
    file << "         BANK ATM RECEIPT               \n";
    file << "========================================\n";
    file << "  Date/Time : " << t.dateTime << "\n";
    file << "  Receipt # : " << t.transId << "\n";
    file << "----------------------------------------\n";
    file << "  Account   : " << t.accNumber << "\n";
    file << "  Name      : " << holderName << "\n";
    file << "  Type      : " << t.type << "\n";
    file << "  Amount    : " << formatMoney(t.amount) << "\n";
    if (t.targetAccNumber != -1) file << "  To Account: " << t.targetAccNumber << "\n";
    file << "  Balance   : " << formatMoney(t.balanceAfter) << "\n";
    file << "========================================\n";
    file << "  Thank you for banking with us!        \n";
    file << "========================================\n";
    file.close();
}

// --- Bank Admin Module (Huzaifa) ---

int createAccount(const string& name, const string& cnic, const string& phone,
    const string& address, const string& accType, double initialDeposit, int pin) {
    if (name.empty() || cnic.length() != 13 || phone.length() != 11 || address.empty()) return -1;
    if (accType != "Savings" && accType != "Current") return -1;
    if (initialDeposit < 0) return -1;
    if (pin < 1000 || pin > 9999) return -1;

    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].cnic == cnic && accounts[i].accType == accType) return -2; 
    }

    Account newAcc;
    newAcc.accNumber = getNextAccountNumber();
    newAcc.holderName = name;
    newAcc.cnic = cnic;
    newAcc.phone = phone;
    newAcc.address = address;
    newAcc.accType = accType;
    newAcc.balance = initialDeposit;
    newAcc.pin = encodePin(pin);
    newAcc.isActive = true;
    newAcc.failedAttempts = 0;
    newAcc.dailyWithdrawn = 0.0;
    newAcc.lastWithdrawDate = "";
    newAcc.hasLoan = false;
    newAcc.loanAmount = 0.0;
    newAcc.loanPaid = 0.0;
    newAcc.interestRate = 0.0;
    newAcc.loanMonths = 0;
    newAcc.loanStartDate = "";

    accounts.push_back(newAcc);
    saveAllAccounts(accounts);
    saveAuditEntry("Account Created", newAcc.accNumber, "Admin", "New " + accType + " account");

    if (initialDeposit > 0) {
        Transaction t;
        t.transId = getNextTransactionId();
        t.accNumber = newAcc.accNumber;
        t.type = "Deposit";
        t.amount = initialDeposit;
        t.dateTime = getCurrentDateTime();
        t.balanceAfter = initialDeposit;
        t.targetAccNumber = -1;
        saveTransaction(t);
    }
    return newAcc.accNumber;
}

bool updateAccount(int accNumber, const string& phone, const string& address) {
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber) {
            if (!phone.empty()) accounts[i].phone = phone;
            if (!address.empty()) accounts[i].address = address;
            saveAllAccounts(accounts);
            saveAuditEntry("Account Updated", accNumber, "Admin", "Details modified");
            return true;
        }
    }
    return false;
}

bool deleteAccount(int accNumber) {
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber) {
            if (accounts[i].hasLoan && accounts[i].loanPaid < accounts[i].loanAmount) return false;
            accounts.erase(accounts.begin() + i);
            saveAllAccounts(accounts);
            saveAuditEntry("Account Deleted", accNumber, "Admin", "Removed permanently");
            return true;
        }
    }
    return false;
}

bool toggleAccountStatus(int accNumber, bool activate) {
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber) {
            accounts[i].isActive = activate;
            if (activate) accounts[i].failedAttempts = 0;
            saveAllAccounts(accounts);
            saveAuditEntry(activate ? "Account Activated" : "Account Deactivated", accNumber, "Admin", "Status changed");
            return true;
        }
    }
    return false;
}

// --- ATM Customer Module (Abdullah) ---
// Abdullah: this handles all the ATM math and daily limits

int authenticateUser(int accNumber, int enteredPin) {
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber) {
            if (!accounts[i].isActive) return 4; 
            if (accounts[i].failedAttempts >= MAX_PIN_ATTEMPTS) return 2;
            
            if (enteredPin != decodePin(accounts[i].pin)) {
                accounts[i].failedAttempts++;
                if (accounts[i].failedAttempts >= MAX_PIN_ATTEMPTS) {
                    accounts[i].isActive = false;
                    saveAuditEntry("Account Auto-Locked", accNumber, "ATM", "Max PIN attempts");
                }
                saveAllAccounts(accounts);
                return 1;
            }
            accounts[i].failedAttempts = 0;
            saveAllAccounts(accounts);
            saveAuditEntry("ATM Login", accNumber, "ATM", "Success");
            return 0;
        }
    }
    return 3;
}

int withdrawMoney(int accNumber, double amount) {
    if (amount <= 0 || (int)amount % 100 != 0) return 1;
    
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber) {
            string today = getCurrentDate();
            if (accounts[i].lastWithdrawDate != today) {
                accounts[i].dailyWithdrawn = 0.0;
                accounts[i].lastWithdrawDate = today;
            }
            if (accounts[i].dailyWithdrawn + amount > DAILY_WITHDRAW_LIMIT) return 3;
            if (accounts[i].balance < amount) return 2;
            
            ATMCash cash = loadInventory();
            int remaining = (int)amount;
            int use5000 = min(remaining / 5000, cash.notes5000); remaining -= use5000 * 5000;
            int use1000 = min(remaining / 1000, cash.notes1000); remaining -= use1000 * 1000;
            int use500 = min(remaining / 500, cash.notes500); remaining -= use500 * 500;
            int use100 = min(remaining / 100, cash.notes100); remaining -= use100 * 100;
            
            if (remaining > 0) return 4;
            
            cash.notes5000 -= use5000; cash.notes1000 -= use1000;
            cash.notes500 -= use500; cash.notes100 -= use100;
            saveInventory(cash);
            
            accounts[i].balance -= amount;
            accounts[i].dailyWithdrawn += amount;
            saveAllAccounts(accounts);
            
            Transaction t;
            t.transId = getNextTransactionId(); t.accNumber = accNumber; t.type = "Withdrawal";
            t.amount = amount; t.dateTime = getCurrentDateTime();
            t.balanceAfter = accounts[i].balance; t.targetAccNumber = -1;
            saveTransaction(t);
            generateReceipt(t, accounts[i].holderName);
            saveAuditEntry("Withdrawal", accNumber, "ATM", "Amount: " + to_string((int)amount));
            return 0;
        }
    }
    return 5;
}

int depositMoney(int accNumber, double amount) {
    if (amount <= 0) return 1;
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber) {
            accounts[i].balance += amount;
            saveAllAccounts(accounts);
            
            Transaction t;
            t.transId = getNextTransactionId(); t.accNumber = accNumber; t.type = "Deposit";
            t.amount = amount; t.dateTime = getCurrentDateTime();
            t.balanceAfter = accounts[i].balance; t.targetAccNumber = -1;
            saveTransaction(t);
            generateReceipt(t, accounts[i].holderName);
            saveAuditEntry("Deposit", accNumber, "ATM", "Amount: " + to_string((int)amount));
            return 0;
        }
    }
    return 2;
}

int transferMoney(int fromAcc, int toAcc, double amount, bool otpVerified) {
    if (amount <= 0 || fromAcc == toAcc) return 1;
    if (amount >= OTP_THRESHOLD && !otpVerified) return 6;
    
    vector<Account> accounts = loadAccounts();
    int fromIdx = -1, toIdx = -1;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == fromAcc) fromIdx = i;
        if (accounts[i].accNumber == toAcc) toIdx = i;
    }
    if (fromIdx == -1) return 5;
    if (toIdx == -1 || !accounts[toIdx].isActive) return 3;
    if (accounts[fromIdx].balance < amount) return 2;
    
    accounts[fromIdx].balance -= amount;
    accounts[toIdx].balance += amount;
    saveAllAccounts(accounts);
    
    Transaction t1;
    t1.transId = getNextTransactionId(); t1.accNumber = fromAcc; t1.type = "Transfer-Out";
    t1.amount = amount; t1.dateTime = getCurrentDateTime();
    t1.balanceAfter = accounts[fromIdx].balance; t1.targetAccNumber = toAcc;
    saveTransaction(t1);
    
    Transaction t2;
    t2.transId = getNextTransactionId(); t2.accNumber = toAcc; t2.type = "Transfer-In";
    t2.amount = amount; t2.dateTime = getCurrentDateTime();
    t2.balanceAfter = accounts[toIdx].balance; t2.targetAccNumber = fromAcc;
    saveTransaction(t2);
    
    generateReceipt(t1, accounts[fromIdx].holderName);
    saveAuditEntry("Transfer", fromAcc, "ATM", "To: " + to_string(toAcc));
    return 0;
}

double calculateSavingsProfit(int accNumber) {
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accNumber == accNumber && accounts[i].accType == "Savings") {
            return accounts[i].balance * SAVINGS_INTEREST_RATE / 100.0;
        }
    }
    return 0.0;
}
// --- SFML GUI Stuff ---
// Abdullah: used AI to help structure these UI classes so we didn't have to do the annoying math for the mouse clicks manually

struct Button {
    RectangleShape rect;
    Text text;
    bool isHovered = false;

    Button() {}
    Button(float x, float y, float w, float h, string label, Font& font, Color bgColor) {
        rect.setPosition(x, y);
        rect.setSize(Vector2f(w, h));
        rect.setFillColor(bgColor);
        rect.setOutlineThickness(1);
        rect.setOutlineColor(Color(200, 200, 200));

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(18);
        text.setFillColor(Color::White);
        
        FloatRect textBounds = text.getLocalBounds();
        text.setPosition(x + (w - textBounds.width) / 2, y + (h - textBounds.height) / 2 - 5);
    }

    void update(Vector2f mousePos) {
        isHovered = rect.getGlobalBounds().contains(mousePos);
        if (isHovered) {
            Color c = rect.getFillColor();
            rect.setOutlineColor(Color::White);
            rect.setOutlineThickness(2);
        } else {
            rect.setOutlineColor(Color(200, 200, 200));
            rect.setOutlineThickness(1);
        }
    }

    void draw(RenderWindow& window) {
        window.draw(rect);
        window.draw(text);
    }
};

struct TextBox {
    RectangleShape rect;
    Text text;
    Text placeholderText;
    string content;
    bool isSelected = false;
    bool isPassword = false;
    int maxLength = 20;

    TextBox() {}
    TextBox(float x, float y, float w, float h, string placeholder, Font& font, bool password = false) {
        rect.setPosition(x, y);
        rect.setSize(Vector2f(w, h));
        rect.setFillColor(Color::White);
        rect.setOutlineThickness(1);
        rect.setOutlineColor(Color(150, 150, 150));

        isPassword = password;
        
        placeholderText.setFont(font);
        placeholderText.setString(placeholder);
        placeholderText.setCharacterSize(16);
        placeholderText.setFillColor(Color(150, 150, 150));
        placeholderText.setPosition(x + 10, y + 10);

        text.setFont(font);
        text.setCharacterSize(16);
        text.setFillColor(Color::Black);
        text.setPosition(x + 10, y + 10);
    }

    void update(Vector2f mousePos, bool clicked) {
        if (clicked) {
            isSelected = rect.getGlobalBounds().contains(mousePos);
            rect.setOutlineColor(isSelected ? Color(0, 120, 215) : Color(150, 150, 150));
            rect.setOutlineThickness(isSelected ? 2 : 1);
        }
    }

    void handleInput(Event event) {
        if (!isSelected) return;
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == 8 && content.length() > 0) { // Backspace
                content.pop_back();
            } else if (event.text.unicode >= 32 && event.text.unicode < 127 && content.length() < maxLength) {
                content += static_cast<char>(event.text.unicode);
            }
            
            if (isPassword) {
                string hidden(content.length(), '*');
                text.setString(hidden);
            } else {
                text.setString(content);
            }
        }
    }

    void draw(RenderWindow& window) {
        window.draw(rect);
        if (content.empty() && !isSelected) {
            window.draw(placeholderText);
        } else {
            window.draw(text);
        }
    }
};

void drawText(RenderWindow& win, string str, float x, float y, Font& font, int size, Color color) {
    Text text;
    text.setFont(font);
    text.setString(str);
    text.setCharacterSize(size);
    text.setFillColor(color);
    text.setPosition(x, y);
    win.draw(text);
}
// --- MAIN EVENT LOOP (Abdullah) ---
// Abdullah: connecting everything together here.

enum AppState {
    MAIN_MENU,
    ADMIN_LOGIN,
    ADMIN_DASHBOARD,
    ADMIN_CREATE_ACC,
    ADMIN_VIEW_ACCS,
    ATM_LOGIN,
    ATM_DASHBOARD,
    ATM_WITHDRAW,
    ATM_DEPOSIT,
    ATM_TRANSFER,
    ATM_MINI_STATEMENT
};

int main() {
    RenderWindow window(VideoMode(900, 600), "Banking System & ATM Simulation");
    window.setFramerateLimit(60);

    Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        cout << "Failed to load font\n";
        return -1;
    }

    AppState state = MAIN_MENU;
    int loggedInAccNumber = -1;
    string systemMessage = "";
    Color primaryColor(26, 54, 80); // Professional Navy Blue
    Color bg(239, 242, 245); // Light Gray

    // setting up all the buttons (took forever to align these properly)
    Button btnAdmin(300, 200, 300, 50, "Bank Administration", font, primaryColor);
    Button btnATM(300, 280, 300, 50, "ATM Customer", font, primaryColor);
    Button btnExit(300, 360, 300, 50, "Exit System", font, Color(180, 50, 50));
    Button btnBack(20, 20, 100, 40, "Back", font, Color(100, 100, 100));

    TextBox txtAdminPass(300, 250, 300, 40, "Admin Password", font, true);
    Button btnAdminLogin(300, 320, 300, 50, "Login", font, primaryColor);
    
    // Admin dashboard
    Button btnCreateAcc(100, 150, 300, 50, "Create Account", font, primaryColor);
    Button btnViewAccs(500, 150, 300, 50, "View All Accounts", font, primaryColor);
    Button btnRestock(100, 250, 300, 50, "Restock ATM", font, primaryColor);

    // ATM dashboard
    Button btnWithdraw(100, 150, 300, 50, "Withdraw Cash", font, primaryColor);
    Button btnDeposit(500, 150, 300, 50, "Deposit Cash", font, primaryColor);
    Button btnTransfer(100, 250, 300, 50, "Transfer Funds", font, primaryColor);
    Button btnMini(500, 250, 300, 50, "Mini Statement", font, primaryColor);

    // Input fields
    TextBox txtAccName(300, 120, 300, 40, "Holder Name", font);
    TextBox txtCnic(300, 180, 300, 40, "CNIC (13 digits)", font);
    TextBox txtPhone(300, 240, 300, 40, "Phone (11 digits)", font);
    TextBox txtAddress(300, 300, 300, 40, "Address", font);
    TextBox txtPin(300, 360, 300, 40, "Set PIN (4 digits)", font, true);
    Button btnSubmitAcc(300, 430, 300, 50, "Create", font, primaryColor);

    TextBox txtAtmAccNo(300, 220, 300, 40, "Account Number", font);
    TextBox txtAtmPin(300, 280, 300, 40, "PIN Code", font, true);
    Button btnAtmLogin(300, 350, 300, 50, "Login", font, primaryColor);

    TextBox txtAmount(300, 250, 300, 40, "Amount (Rs.)", font);
    TextBox txtTargetAcc(300, 310, 300, 40, "Target Account #", font);
    Button btnAction(300, 380, 300, 50, "Submit", font, primaryColor);

    while (window.isOpen()) {
        Vector2i m = Mouse::getPosition(window);
        Vector2f mousePos(m.x, m.y);
        bool clicked = false;
        
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();
            if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) clicked = true;
            
            if (state == ADMIN_LOGIN) txtAdminPass.handleInput(event);
            if (state == ADMIN_CREATE_ACC) { txtAccName.handleInput(event); txtCnic.handleInput(event); txtPhone.handleInput(event); txtAddress.handleInput(event); txtPin.handleInput(event); }
            if (state == ATM_LOGIN) { txtAtmAccNo.handleInput(event); txtAtmPin.handleInput(event); }
            if (state == ATM_WITHDRAW || state == ATM_DEPOSIT || state == ATM_TRANSFER) {
                txtAmount.handleInput(event);
                if (state == ATM_TRANSFER) txtTargetAcc.handleInput(event);
            }
        }

        // Huzaifa: bro make sure the back button doesn't crash here
        // Abdullah: fixed it, it resets the state properly now
        if (state != MAIN_MENU) btnBack.update(mousePos);
        if (clicked && state != MAIN_MENU && btnBack.isHovered) {
            systemMessage = "";
            if (state == ADMIN_DASHBOARD || state == ATM_LOGIN || state == ADMIN_LOGIN) state = MAIN_MENU;
            else if (state == ADMIN_CREATE_ACC || state == ADMIN_VIEW_ACCS) state = ADMIN_DASHBOARD;
            else if (state == ATM_DASHBOARD) state = MAIN_MENU;
            else state = ATM_DASHBOARD;
            loggedInAccNumber = -1;
        }

        if (state == MAIN_MENU) {
            btnAdmin.update(mousePos); btnATM.update(mousePos); btnExit.update(mousePos);
            if (clicked) {
                if (btnAdmin.isHovered) { state = ADMIN_LOGIN; txtAdminPass.content = ""; txtAdminPass.text.setString(""); systemMessage = ""; }
                if (btnATM.isHovered) { state = ATM_LOGIN; txtAtmAccNo.content = ""; txtAtmPin.content = ""; txtAtmAccNo.text.setString(""); txtAtmPin.text.setString(""); systemMessage = ""; }
                if (btnExit.isHovered) window.close();
            }
        } else if (state == ADMIN_LOGIN) {
            txtAdminPass.update(mousePos, clicked); btnAdminLogin.update(mousePos);
            if (clicked && btnAdminLogin.isHovered) {
                if (txtAdminPass.content == "bank2026") { state = ADMIN_DASHBOARD; systemMessage = ""; }
                else systemMessage = "Incorrect Admin Password!";
            }
        } else if (state == ADMIN_DASHBOARD) {
            btnCreateAcc.update(mousePos); btnViewAccs.update(mousePos); btnRestock.update(mousePos);
            if (clicked) {
                if (btnCreateAcc.isHovered) {
                    state = ADMIN_CREATE_ACC;
                    txtAccName.content = ""; txtCnic.content = ""; txtPhone.content = ""; txtAddress.content = ""; txtPin.content = "";
                    txtAccName.text.setString(""); txtCnic.text.setString(""); txtPhone.text.setString(""); txtAddress.text.setString(""); txtPin.text.setString("");
                    systemMessage = "";
                }
                if (btnViewAccs.isHovered) state = ADMIN_VIEW_ACCS;
                if (btnRestock.isHovered) {
                    ATMCash cash = loadInventory();
                    cash.notes5000 += 100; cash.notes1000 += 100; cash.notes500 += 100; cash.notes100 += 100;
                    saveInventory(cash);
                    systemMessage = "ATM Restocked Successfully!";
                }
            }
        } else if (state == ADMIN_CREATE_ACC) {
            txtAccName.update(mousePos, clicked); txtCnic.update(mousePos, clicked);
            txtPhone.update(mousePos, clicked); txtAddress.update(mousePos, clicked);
            txtPin.update(mousePos, clicked); btnSubmitAcc.update(mousePos);
            if (clicked && btnSubmitAcc.isHovered) {
                int p = toInt(txtPin.content);
                int accNo = createAccount(txtAccName.content, txtCnic.content, txtPhone.content, txtAddress.content, "Savings", 0, p);
                if (accNo > 0) { systemMessage = "Success! Account Number: " + to_string(accNo); state = ADMIN_DASHBOARD; }
                else systemMessage = "Failed. Check inputs (13 digit CNIC, 11 digit Phone).";
            }
        } else if (state == ATM_LOGIN) {
            txtAtmAccNo.update(mousePos, clicked); txtAtmPin.update(mousePos, clicked); btnAtmLogin.update(mousePos);
            if (clicked && btnAtmLogin.isHovered) {
                int acc = toInt(txtAtmAccNo.content);
                int p = toInt(txtAtmPin.content);
                int res = authenticateUser(acc, p);
                if (res == 0) { loggedInAccNumber = acc; state = ATM_DASHBOARD; systemMessage = ""; }
                else if (res == 1) systemMessage = "Incorrect PIN!";
                else if (res == 2) systemMessage = "Account locked due to max attempts!";
                else systemMessage = "Account not found or inactive!";
            }
        } else if (state == ATM_DASHBOARD) {
            btnWithdraw.update(mousePos); btnDeposit.update(mousePos); btnTransfer.update(mousePos); btnMini.update(mousePos);
            if (clicked) {
                if (btnWithdraw.isHovered) { state = ATM_WITHDRAW; txtAmount.content = ""; txtAmount.text.setString(""); systemMessage = ""; }
                if (btnDeposit.isHovered) { state = ATM_DEPOSIT; txtAmount.content = ""; txtAmount.text.setString(""); systemMessage = ""; }
                if (btnTransfer.isHovered) { state = ATM_TRANSFER; txtAmount.content = ""; txtTargetAcc.content = ""; txtAmount.text.setString(""); txtTargetAcc.text.setString(""); systemMessage = ""; }
                if (btnMini.isHovered) state = ATM_MINI_STATEMENT;
            }
        } else if (state == ATM_WITHDRAW || state == ATM_DEPOSIT || state == ATM_TRANSFER) {
            txtAmount.update(mousePos, clicked); btnAction.update(mousePos);
            if (state == ATM_TRANSFER) txtTargetAcc.update(mousePos, clicked);
            if (clicked && btnAction.isHovered) {
                double amt = toDouble(txtAmount.content);
                if (state == ATM_WITHDRAW) {
                    int res = withdrawMoney(loggedInAccNumber, amt);
                    if (res == 0) systemMessage = "Withdrawal successful! Receipt generated.";
                    else if (res == 2) systemMessage = "Insufficient balance!";
                    else if (res == 4) systemMessage = "ATM is out of cash!";
                    else systemMessage = "Invalid amount!";
                } else if (state == ATM_DEPOSIT) {
                    int res = depositMoney(loggedInAccNumber, amt);
                    if (res == 0) systemMessage = "Deposit successful! Receipt generated.";
                    else systemMessage = "Invalid amount!";
                } else if (state == ATM_TRANSFER) {
                    int target = toInt(txtTargetAcc.content);
                    int res = transferMoney(loggedInAccNumber, target, amt, false); // basic, no OTP logic in this simplified flow
                    if (res == 0) systemMessage = "Transfer successful! Receipt generated.";
                    else if (res == 2) systemMessage = "Insufficient balance!";
                    else if (res == 3) systemMessage = "Target account not found!";
                    else systemMessage = "Invalid transfer details!";
                }
                state = ATM_DASHBOARD;
            }
        }

        // Abdullah: finally drawing everything to the screen
        window.clear(bg);
        
        // Header
        RectangleShape header(Vector2f(900, 80));
        header.setFillColor(primaryColor);
        window.draw(header);
        drawText(window, "Global Bank System", 20, 25, font, 28, Color::White);
        
        if (!systemMessage.empty()) {
            drawText(window, systemMessage, 300, 520, font, 18, Color::Red);
        }

        if (state != MAIN_MENU) btnBack.draw(window);

        if (state == MAIN_MENU) {
            btnAdmin.draw(window); btnATM.draw(window); btnExit.draw(window);
        } else if (state == ADMIN_LOGIN) {
            drawText(window, "Admin Authentication", 350, 200, font, 22, primaryColor);
            txtAdminPass.draw(window); btnAdminLogin.draw(window);
        } else if (state == ADMIN_DASHBOARD) {
            drawText(window, "Administrator Dashboard", 320, 100, font, 24, primaryColor);
            btnCreateAcc.draw(window); btnViewAccs.draw(window); btnRestock.draw(window);
        } else if (state == ADMIN_CREATE_ACC) {
            drawText(window, "Create New Account", 350, 80, font, 22, primaryColor);
            txtAccName.draw(window); txtCnic.draw(window); txtPhone.draw(window); txtAddress.draw(window); txtPin.draw(window); btnSubmitAcc.draw(window);
        } else if (state == ADMIN_VIEW_ACCS) {
            drawText(window, "All Accounts Data", 350, 100, font, 24, primaryColor);
            vector<Account> accs = loadAccounts();
            int y = 150;
            for (size_t i = 0; i < accs.size() && i < 10; i++) {
                drawText(window, "Acc: " + to_string(accs[i].accNumber) + " | " + accs[i].holderName + " | Bal: " + formatMoney(accs[i].balance) + " | " + (accs[i].isActive ? "Active" : "Locked"), 100, y, font, 16, Color::Black);
                y += 30;
            }
        } else if (state == ATM_LOGIN) {
            drawText(window, "ATM Authentication", 350, 150, font, 24, primaryColor);
            txtAtmAccNo.draw(window); txtAtmPin.draw(window); btnAtmLogin.draw(window);
        } else if (state == ATM_DASHBOARD) {
            vector<Account> accounts = loadAccounts();
            string n = ""; double bal = 0;
            for (size_t i=0; i<accounts.size(); i++) { if (accounts[i].accNumber == loggedInAccNumber) { n = accounts[i].holderName; bal = accounts[i].balance; break; } }
            drawText(window, "Welcome, " + n, 350, 100, font, 24, primaryColor);
            drawText(window, "Current Balance: " + formatMoney(bal), 320, 400, font, 22, Color(0, 120, 0));
            btnWithdraw.draw(window); btnDeposit.draw(window); btnTransfer.draw(window); btnMini.draw(window);
        } else if (state == ATM_WITHDRAW || state == ATM_DEPOSIT || state == ATM_TRANSFER) {
            string title = state == ATM_WITHDRAW ? "Withdraw Cash" : (state == ATM_DEPOSIT ? "Deposit Cash" : "Transfer Funds");
            drawText(window, title, 350, 150, font, 24, primaryColor);
            txtAmount.draw(window);
            if (state == ATM_TRANSFER) txtTargetAcc.draw(window);
            btnAction.draw(window);
        } else if (state == ATM_MINI_STATEMENT) {
            drawText(window, "Mini Statement (Last 5)", 300, 100, font, 24, primaryColor);
            vector<Transaction> all = loadTransactions();
            int y = 150, count = 0;
            for (int i = all.size() - 1; i >= 0 && count < 5; i--) {
                if (all[i].accNumber == loggedInAccNumber) {
                    drawText(window, all[i].dateTime + " | " + all[i].type + " | " + formatMoney(all[i].amount) + " | Bal: " + formatMoney(all[i].balanceAfter), 100, y, font, 16, Color::Black);
                    y += 35;
                    count++;
                }
            }
        }

        window.display();
    }
    return 0;
}




