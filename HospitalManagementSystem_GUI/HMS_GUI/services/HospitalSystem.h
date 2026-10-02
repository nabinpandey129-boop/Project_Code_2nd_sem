#pragma once
#include <fstream>
#include <memory>
#include <filesystem>
#include "../models/User.h"
#include "../models/Patient.h"
#include "../models/Appointment.h"
#include "../models/MedicalVisit.h"
#include "../models/Test.h"
#include "../models/Procedure.h"
#include "../models/Bill.h"

// Holds all data, loads/saves the text files in data/, and contains the business logic
class HospitalSystem {
    vector<unique_ptr<User>> users;
    vector<Patient> patients;
    vector<Appointment> appts;
    vector<MedicalVisit> visits;
    vector<unique_ptr<Service>> services;
    vector<Bill> bills;
    const string dir = "data/";

    static vector<string> readLines(const string& file) {
        ifstream in(file); vector<string> r; string l;
        while (getline(in, l)) if (!l.empty()) r.push_back(l);
        return r;
    }
    template <class T> static string nextId(const vector<T>& v, const string& p, int w) {
        int m = 0;
        for (auto& x : v) m = max(m, atoi(x.getId().substr(p.size()).c_str()));
        return p + pad(m + 1, w);
    }
    static unique_ptr<User> makeUser(const string& line) {
        auto f = split(line, '|', 7);   // role|id|username|password|name|extra1|extra2
        if (f[0] == "Admin") return make_unique<Admin>(f[1], f[2], f[3], f[4]);
        if (f[0] == "Doctor") return make_unique<Doctor>(f[1], f[2], f[3], f[4], f[5], atoi(f[6].c_str()));
        return make_unique<Receptionist>(f[1], f[2], f[3], f[4]);
    }

    void load() {
        for (auto& l : readLines(dir + "users.txt")) users.push_back(makeUser(l));
        for (auto& l : readLines(dir + "patients.txt")) patients.push_back(Patient::fromRecord(l));
        for (auto& l : readLines(dir + "appointments.txt")) appts.push_back(Appointment::fromRecord(l));
        for (auto& l : readLines(dir + "visits.txt")) visits.push_back(MedicalVisit::fromRecord(l));
        for (auto& l : readLines(dir + "bills.txt")) bills.push_back(Bill::fromRecord(l));
        for (auto& l : readLines(dir + "services.txt")) {
            auto f = split(l, '|', 4);
            if (f[0] == "Test") services.push_back(make_unique<Test>(f[1], f[2], atoi(f[3].c_str())));
            else services.push_back(make_unique<Procedure>(f[1], f[2], atoi(f[3].c_str())));
        }
        if (users.empty()) {   // first run: default accounts
            users.push_back(make_unique<Admin>("A001", "admin01", "admin123", "Admin"));
            users.push_back(make_unique<Receptionist>("R001", "recep01", "recep123", "Sita Reception"));
            users.push_back(make_unique<Doctor>("D001", "dr.sharma", "doc123", "Dr. Sharma", "General Physician", 500));
            users.push_back(make_unique<Doctor>("D002", "dr.rai", "doc123", "Dr. Rai", "Pediatrician", 600));
        }
        if (services.empty()) {   // first run: predefined services
            services.push_back(make_unique<Test>("T001", "Blood Test", 300));
            services.push_back(make_unique<Test>("T002", "X-Ray", 800));
            services.push_back(make_unique<Test>("T003", "Urine Test", 200));
            services.push_back(make_unique<Procedure>("PR001", "Injection", 150));
            services.push_back(make_unique<Procedure>("PR002", "Dressing", 250));
            services.push_back(make_unique<Procedure>("PR003", "Minor Procedure", 1000));
        }
        save();
    }
    template <class C> void writeAll(const string& file, const C& c) {
        ofstream out(dir + file);
        for (auto& x : c) out << x.toRecord() << "\n";
    }
public:
    HospitalSystem() { filesystem::create_directories(dir); load(); }
    ~HospitalSystem() { save(); }

    void save() {
        { ofstream o(dir + "users.txt"); for (auto& u : users) o << u->toRecord() << "\n"; }
        { ofstream o(dir + "services.txt"); for (auto& s : services) o << s->toRecord() << "\n"; }
        writeAll("patients.txt", patients);
        writeAll("appointments.txt", appts);
        writeAll("visits.txt", visits);
        writeAll("bills.txt", bills);
    }

    // ---------- Authentication ----------
    User* login(const string& username, const string& password) {
        for (auto& u : users) if (u->getUsername() == username && u->checkPassword(password)) return u.get();
        return nullptr;
    }

    // ---------- Users ----------
    vector<User*> usersByRole(const string& role) {
        vector<User*> r; for (auto& u : users) if (u->role() == role) r.push_back(u.get()); return r;
    }
    vector<Doctor*> doctors() {
        vector<Doctor*> r; for (auto& u : users) if (auto d = dynamic_cast<Doctor*>(u.get())) r.push_back(d); return r;
    }
    Doctor* findDoctor(const string& id) { for (auto d : doctors()) if (d->getId() == id) return d; return nullptr; }
    string doctorName(const string& id) { Doctor* d = findDoctor(id); return d ? d->getName() : "Unknown"; }
    User* findUser(const string& id) { for (auto& u : users) if (u->getId() == id) return u.get(); return nullptr; }
    bool usernameTaken(const string& un) { for (auto& u : users) if (u->getUsername() == un) return true; return false; }
    string nextUserId(char prefix) {
        int m = 0;
        for (auto& u : users) if (u->getId()[0] == prefix) m = max(m, atoi(u->getId().substr(1).c_str()));
        return string(1, prefix) + pad(m + 1, 3);
    }
    void addUser(unique_ptr<User> u) { users.push_back(move(u)); save(); }
    // returns "" on success, or the reason it could not be deleted
    string deleteUser(const string& id) {
        for (auto& a : appts) if (a.getDoctorId() == id) return "This staff member has appointment records.";
        for (auto& v : visits) if (v.getDoctorId() == id) return "This staff member has visit records.";
        for (size_t i = 0; i < users.size(); i++)
            if (users[i]->getId() == id) { users.erase(users.begin() + i); save(); return ""; }
        return "Not found.";
    }

    // ---------- Patients ----------
    vector<Patient>& allPatients() { return patients; }
    Patient* findPatient(const string& id) {
        for (auto& p : patients) { if (lower(p.getId()) == lower(id)) return &p; }
        return nullptr;
    }
    string patientName(const string& id) { Patient* p = findPatient(id); return p ? p->getName() : "Unknown"; }
    vector<Patient*> searchPatients(const string& term) {
        vector<Patient*> r; string t = lower(term);
        for (auto& p : patients)
            if (lower(p.getId()).find(t) != string::npos || lower(p.getName()).find(t) != string::npos ||
                p.getPhone().find(term) != string::npos) r.push_back(&p);
        return r;
    }
    string addPatient(Patient p) { p.setId(nextId(patients, "P", 3)); patients.push_back(p); save(); return p.getId(); }
    void deletePatient(const string& id) {   // also removes the patient's appointments, visits and bills
        auto rm = [&](auto& v) { for (size_t i = v.size(); i-- > 0;) if (lower(v[i].getPatientId()) == lower(id)) v.erase(v.begin() + i); };
        rm(appts); rm(visits); rm(bills);
        for (size_t i = 0; i < patients.size(); i++) if (lower(patients[i].getId()) == lower(id)) { patients.erase(patients.begin() + i); break; }
        save();
    }

    // ---------- Appointments ----------
    vector<Appointment>& allAppointments() { return appts; }
    Appointment* findAppointment(const string& id) { for (auto& a : appts) if (lower(a.getId()) == lower(id)) return &a; return nullptr; }
    bool slotTaken(const string& doc, const string& date, const string& time) {
        for (auto& a : appts) if (a.getDoctorId() == doc && a.getDate() == date && a.getTime() == time && a.getStatus() == "Scheduled") return true;
        return false;
    }
    string addAppointment(Appointment a) { a.setId(nextId(appts, "AP", 3)); appts.push_back(a); save(); return a.getId(); }

    // ---------- Medical history ----------
    vector<MedicalVisit>& allVisits() { return visits; }
    MedicalVisit* findVisit(const string& id) { for (auto& v : visits) if (lower(v.getId()) == lower(id)) return &v; return nullptr; }
    vector<MedicalVisit*> visitsOf(const string& patientId) {
        vector<MedicalVisit*> r; for (auto& v : visits) if (lower(v.getPatientId()) == lower(patientId)) r.push_back(&v); return r;
    }
    // Creates the visit AND its bill (with the doctor's consultation fee); completes the appointment if linked
    string addVisit(MedicalVisit v) {
        v.setId(nextId(visits, "V", 3));
        Doctor* d = findDoctor(v.getDoctorId());
        bills.push_back(Bill(nextId(bills, "B", 3), v.getPatientId(), v.getId(), v.getDate(), d ? d->getFee() : 0));
        if (!v.getAppointmentId().empty()) if (Appointment* a = findAppointment(v.getAppointmentId())) a->setStatus("Completed");
        visits.push_back(v); save();
        return v.getId();
    }

    // ---------- Services ----------
    const vector<unique_ptr<Service>>& allServices() { return services; }
    Service* findService(const string& id) { for (auto& s : services) if (lower(s->getId()) == lower(id)) return s.get(); return nullptr; }

    // Doctor -> Billing integration: looks up price, adds to visit + bill, recalculates total. Returns "" on success.
    string addServiceToVisit(const string& visitId, const string& serviceId) {
        MedicalVisit* v = findVisit(visitId); Service* s = findService(serviceId); Bill* b = billForVisit(visitId);
        if (!v) return "Visit not found.";
        if (!s) return "Service not found.";
        if (!b) return "Bill not found.";
        if (b->isPaid()) return "Bill is already paid; cannot add services.";
        v->addService(s->getId()); b->addItem(s->getName(), s->getPrice(), s->type());
        save(); return "";
    }

    // ---------- Billing ----------
    vector<Bill>& allBills() { return bills; }
    Bill* findBill(const string& id) { for (auto& b : bills) if (lower(b.getId()) == lower(id)) return &b; return nullptr; }
    Bill* billForVisit(const string& visitId) { for (auto& b : bills) if (b.getVisitId() == visitId) return &b; return nullptr; }
};
