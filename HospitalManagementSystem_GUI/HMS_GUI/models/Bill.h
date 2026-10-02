#pragma once
#include "Util.h"

struct BillItem { string name; int price; string kind; };   // kind: Test / Procedure

class Bill {
    string id, patientId, visitId, date;
    int consultationFee, otherCharges;
    bool paid;
    vector<BillItem> items;
public:
    Bill() : consultationFee(0), otherCharges(0), paid(false) {}
    Bill(string i, string p, string v, string d, int fee)
        : id(i), patientId(p), visitId(v), date(d), consultationFee(fee), otherCharges(0), paid(false) {}

    string getId() const { return id; }
    string getPatientId() const { return patientId; }
    string getVisitId() const { return visitId; }
    string getDate() const { return date; }
    int getConsultationFee() const { return consultationFee; }
    int getOtherCharges() const { return otherCharges; }
    bool isPaid() const { return paid; }
    const vector<BillItem>& getItems() const { return items; }
    void setId(const string& v) { id = v; }
    void setPaid(bool p) { paid = p; }
    void setOtherCharges(int c) { otherCharges = c; }
    void addItem(const string& n, int p, const string& k) { items.push_back({clean(n), p, k}); }

    int testCharges() const { int t = 0; for (auto& i : items) if (i.kind == "Test") t += i.price; return t; }
    int procedureCharges() const { int t = 0; for (auto& i : items) if (i.kind == "Procedure") t += i.price; return t; }
    int total() const { return consultationFee + testCharges() + procedureCharges() + otherCharges; }

    string toRecord() const {
        string s;
        for (size_t i = 0; i < items.size(); i++)
            s += (i ? ";" : "") + items[i].name + "~" + to_string(items[i].price) + "~" + items[i].kind;
        return id + "|" + patientId + "|" + visitId + "|" + date + "|" + to_string(consultationFee) + "|" +
               to_string(otherCharges) + "|" + (paid ? "1" : "0") + "|" + s;
    }
    static Bill fromRecord(const string& line) {
        auto f = split(line, '|', 8);
        Bill b(f[0], f[1], f[2], f[3], atoi(f[4].c_str()));
        b.otherCharges = atoi(f[5].c_str());
        b.paid = (f[6] == "1");
        if (!f[7].empty())
            for (auto& it : split(f[7], ';')) {
                auto p = split(it, '~', 3);
                b.items.push_back({p[0], atoi(p[1].c_str()), p[2]});
            }
        return b;
    }
    void print(const string& patientName) const {
        cout << "\n========== BILL REPORT ==========\n"
             << "Bill ID: " << id << "\nPatient: " << patientId << " (" << patientName << ")"
             << "\nVisit  : " << visitId << "\nDate   : " << date << "\n\n"
             << left << setw(22) << "Consultation" << "Rs. " << consultationFee << "\n";
        for (auto& i : items) cout << left << setw(22) << i.name << "Rs. " << i.price << "\n";
        if (otherCharges) cout << left << setw(22) << "Other Charges" << "Rs. " << otherCharges << "\n";
        cout << "-------------------------------\n" << left << setw(22) << "TOTAL" << "Rs. " << total()
             << "\nPayment Status:    " << (paid ? "Paid" : "Unpaid")
             << "\n================================\n";
    }
};
