#pragma once
#include "Util.h"

class MedicalVisit {
    string id, patientId, doctorId, appointmentId, date, diagnosis, notes;
    vector<string> serviceIds;   // tests/procedures suggested during this visit
public:
    MedicalVisit() {}
    MedicalVisit(string i, string p, string d, string a, string dt, string diag, string n)
        : id(i), patientId(p), doctorId(d), appointmentId(a), date(dt), diagnosis(diag), notes(n) {}
    string getId() const { return id; }
    string getPatientId() const { return patientId; }
    string getDoctorId() const { return doctorId; }
    string getAppointmentId() const { return appointmentId; }
    string getDate() const { return date; }
    string getDiagnosis() const { return diagnosis; }
    string getNotes() const { return notes; }
    const vector<string>& getServiceIds() const { return serviceIds; }
    void setId(const string& v) { id = v; }
    void addService(const string& sid) { serviceIds.push_back(sid); }
    string toRecord() const {
        string s;
        for (size_t i = 0; i < serviceIds.size(); i++) s += (i ? "," : "") + serviceIds[i];
        return id + "|" + patientId + "|" + doctorId + "|" + appointmentId + "|" + date + "|" + diagnosis + "|" + notes + "|" + s;
    }
    static MedicalVisit fromRecord(const string& line) {
        auto f = split(line, '|', 8);
        MedicalVisit v(f[0], f[1], f[2], f[3], f[4], f[5], f[6]);
        if (!f[7].empty()) for (auto& s : split(f[7], ',')) v.addService(s);
        return v;
    }
};
