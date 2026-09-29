#pragma once
#include "Util.h"

class Appointment {
    string id, patientId, doctorId, date, time, reason, status;   // status: Scheduled / Completed / Cancelled
public:
    Appointment() {}
    Appointment(string i, string p, string d, string dt, string t, string r, string s = "Scheduled")
        : id(i), patientId(p), doctorId(d), date(dt), time(t), reason(r), status(s) {}
    string getId() const { return id; }
    string getPatientId() const { return patientId; }
    string getDoctorId() const { return doctorId; }
    string getDate() const { return date; }
    string getTime() const { return time; }
    string getReason() const { return reason; }
    string getStatus() const { return status; }
    void setId(const string& v) { id = v; }
    void setStatus(const string& v) { status = v; }
    string toRecord() const {
        return id + "|" + patientId + "|" + doctorId + "|" + date + "|" + time + "|" + reason + "|" + status;
    }
    static Appointment fromRecord(const string& line) {
        auto f = split(line, '|', 7);
        return Appointment(f[0], f[1], f[2], f[3], f[4], f[5], f[6]);
    }
};
