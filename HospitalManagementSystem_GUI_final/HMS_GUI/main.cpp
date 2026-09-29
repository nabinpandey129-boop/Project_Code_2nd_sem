// Hospital Management System - GUI edition.
// C++ backend (OOP classes in models/ + services/) serving a browser-based interface.
#include "httplib.h"
#include "services/HospitalSystem.h"
#include "web/Page.h"
#include <mutex>
#include <map>
#include <random>
#include <functional>
#include <algorithm>

using namespace httplib;

static HospitalSystem sys;
static mutex mtx;
static map<string, string> sessions;   // token -> user id

// ---------- tiny JSON helpers ----------
static string q(const string& s) {
    string o = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\t': o += "\\t"; break;
            default: if (c >= 0x20) o += (char)c;
        }
    }
    return o + "\"";
}
struct Obj {
    string s = "{"; bool first = true;
    Obj& raw(const string& k, const string& v) { if (!first) s += ","; first = false; s += q(k) + ":" + v; return *this; }
    Obj& str(const string& k, const string& v) { return raw(k, q(v)); }
    Obj& num(const string& k, long v) { return raw(k, to_string(v)); }
    Obj& boolean(const string& k, bool b) { return raw(k, b ? "true" : "false"); }
    string done() const { return s + "}"; }
};
static string arr(const vector<string>& v) {
    string s = "[";
    for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + v[i];
    return s + "]";
}
static void ok(Response& r, const string& body) { r.set_content(body, "application/json; charset=utf-8"); }
static void fail(Response& r, int code, const string& msg) {
    r.status = code; r.set_content(Obj().str("error", msg).done(), "application/json; charset=utf-8");
}
static string P(const Request& r, const char* k) {   // trimmed request parameter
    string v = r.has_param(k) ? r.get_param_value(k) : "";
    size_t a = v.find_first_not_of(" \t\r\n"), b = v.find_last_not_of(" \t\r\n");
    return a == string::npos ? "" : clean(v.substr(a, b - a + 1));
}

// ---------- JSON for model objects ----------
static string patientJ(const Patient& p) {
    return Obj().str("id", p.getId()).str("name", p.getName()).str("dob", p.getDob()).num("age", p.age())
        .str("gender", p.getGender()).str("phone", p.getPhone()).str("address", p.getAddress())
        .str("blood", p.getBloodGroup()).str("emergency", p.getEmergency()).done();
}
static string apptJ(const Appointment& a) {
    return Obj().str("id", a.getId()).str("patientId", a.getPatientId()).str("patientName", sys.patientName(a.getPatientId()))
        .str("doctorId", a.getDoctorId()).str("doctorName", sys.doctorName(a.getDoctorId())).str("date", a.getDate())
        .str("time", a.getTime()).str("reason", a.getReason()).str("status", a.getStatus()).done();
}
static string serviceJ(const Service& s) {
    return Obj().str("id", s.getId()).str("name", s.getName()).num("price", s.getPrice()).str("type", s.type()).done();
}
static string visitJ(const MedicalVisit& v) {
    vector<string> sv;
    for (auto& id : v.getServiceIds()) if (Service* s = sys.findService(id)) sv.push_back(serviceJ(*s));
    return Obj().str("id", v.getId()).str("date", v.getDate()).str("doctorId", v.getDoctorId())
        .str("doctorName", sys.doctorName(v.getDoctorId())).str("diagnosis", v.getDiagnosis()).str("notes", v.getNotes())
        .raw("services", arr(sv)).done();
}
static string billJ(const Bill& b) {
    vector<string> items;
    for (auto& i : b.getItems()) items.push_back(Obj().str("name", i.name).num("price", i.price).str("kind", i.kind).done());
    return Obj().str("id", b.getId()).str("patientId", b.getPatientId()).str("patientName", sys.patientName(b.getPatientId()))
        .str("visitId", b.getVisitId()).str("date", b.getDate()).num("consultation", b.getConsultationFee())
        .num("tests", b.testCharges()).num("procedures", b.procedureCharges()).num("other", b.getOtherCharges())
        .num("total", b.total()).boolean("paid", b.isPaid()).raw("items", arr(items)).done();
}
static string userJ(User* u) {
    Obj o; o.str("id", u->getId()).str("username", u->getUsername()).str("name", u->getName()).str("role", u->role());
    if (auto d = dynamic_cast<Doctor*>(u)) o.str("specialization", d->getSpecialization()).num("fee", d->getFee());
    return o.done();
}
static string meJ(User* u) {
    Obj o; o.str("id", u->getId()).str("name", u->getName()).str("role", u->role()).raw("pages", u->pages());   // polymorphic: role decides pages
    if (auto d = dynamic_cast<Doctor*>(u)) o.num("fee", d->getFee());
    return o.done();
}

// ---------- auth / routing helpers ----------
static User* currentUser(const Request& req) {
    auto it = sessions.find(req.get_header_value("X-Token"));
    return it == sessions.end() ? nullptr : sys.findUser(it->second);
}
using Fn = function<void(const Request&, Response&, User*)>;
static Server::Handler route(vector<string> roles, Fn fn) {   // login check + role permission check + lock
    return [=](const Request& req, Response& res) {
        lock_guard<mutex> lk(mtx);
        User* u = currentUser(req);
        if (!u) return fail(res, 401, "Please log in.");
        if (!roles.empty() && find(roles.begin(), roles.end(), u->role()) == roles.end())
            return fail(res, 403, "Your role is not allowed to do this.");
        fn(req, res, u);
    };
}
static const vector<string> ALL = {}, AR = {"Admin", "Receptionist"}, ADMIN = {"Admin"}, DOC = {"Doctor"};
static const vector<string> ALLROLES = {"Admin", "Receptionist", "Doctor"};

static string newToken() {
    static mt19937_64 g((random_device())() ^ (unsigned long long)time(nullptr) ^ ((unsigned long long)clock() << 20));
    char b[33]; snprintf(b, sizeof b, "%016llx%016llx", (unsigned long long)g(), (unsigned long long)g()); return b;
}

int main() {
    Server svr;

    svr.Get("/", [](const Request&, Response& res) { res.set_content(kPage, "text/html; charset=utf-8"); });

    // ----- authentication -----
    svr.Post("/api/login", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(mtx);
        User* u = sys.login(P(req, "username"), req.has_param("password") ? req.get_param_value("password") : "");
        if (!u) return fail(res, 401, "Invalid username or password.");
        string t = newToken(); sessions[t] = u->getId();
        string me = meJ(u); me.pop_back();
        ok(res, me + ",\"token\":" + q(t) + "}");
    });
    svr.Get("/api/me", route(ALLROLES, [](const Request&, Response& res, User* u) { ok(res, meJ(u)); }));
    svr.Post("/api/logout", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(mtx); sessions.erase(req.get_header_value("X-Token")); ok(res, "{}");
    });

    // ----- dashboard numbers -----
    svr.Get("/api/summary", route(ALLROLES, [](const Request&, Response& res, User* u) {
        int sched = 0, todayAp = 0, unpaid = 0; string t = today();
        bool doc = u->role() == "Doctor";
        for (auto& a : sys.allAppointments()) {
            if (doc && a.getDoctorId() != u->getId()) continue;
            if (a.getStatus() == "Scheduled") { sched++; if (a.getDate() == t) todayAp++; }
        }
        for (auto& b : sys.allBills()) if (!b.isPaid()) unpaid++;
        Obj o; o.num("patients", sys.allPatients().size()).num("doctors", sys.doctors().size())
            .num("scheduled", sched).num("today", todayAp).num("visits", sys.allVisits().size());
        if (!doc) o.num("unpaid", unpaid);
        ok(res, o.done());
    }));

    // ----- patients -----
    svr.Get("/api/patients", route(ALLROLES, [](const Request& req, Response& res, User*) {
        string term = P(req, "q"); vector<string> out;
        if (term.empty()) for (auto& p : sys.allPatients()) out.push_back(patientJ(p));
        else for (auto p : sys.searchPatients(term)) out.push_back(patientJ(*p));
        ok(res, arr(out));
    }));
    auto readPatient = [](const Request& req, Patient& p, string& err) {
        string name = P(req, "name"), dob = P(req, "dob"), g = P(req, "gender"), ph = P(req, "phone"),
               ad = P(req, "address"), bl = P(req, "blood"), em = P(req, "emergency");
        if (name.empty() || g.empty() || ph.empty() || ad.empty() || bl.empty() || em.empty()) { err = "Please fill in all fields."; return false; }
        if (!isDate(dob)) { err = "Date of birth must be YYYY-MM-DD."; return false; }
        p.setName(name); p.setDob(dob); p.setGender(g); p.setPhone(ph); p.setAddress(ad); p.setBloodGroup(bl); p.setEmergency(em);
        return true;
    };
    svr.Post("/api/patients", route(AR, [=](const Request& req, Response& res, User*) {
        Patient p; string err;
        if (!readPatient(req, p, err)) return fail(res, 400, err);
        ok(res, Obj().str("id", sys.addPatient(p)).done());
    }));
    svr.Post(R"(/api/patients/([^/]+)/update)", route(AR, [=](const Request& req, Response& res, User*) {
        Patient* p = sys.findPatient(req.matches[1]);
        if (!p) return fail(res, 404, "Patient not found.");
        Patient tmp; string err;
        if (!readPatient(req, tmp, err)) return fail(res, 400, err);
        p->setName(tmp.getName()); p->setDob(tmp.getDob()); p->setGender(tmp.getGender()); p->setPhone(tmp.getPhone());
        p->setAddress(tmp.getAddress()); p->setBloodGroup(tmp.getBloodGroup()); p->setEmergency(tmp.getEmergency());
        sys.save(); ok(res, "{}");
    }));
    svr.Post(R"(/api/patients/([^/]+)/delete)", route(ADMIN, [](const Request& req, Response& res, User*) {
        Patient* p = sys.findPatient(req.matches[1]);
        if (!p) return fail(res, 404, "Patient not found.");
        sys.deletePatient(p->getId()); ok(res, "{}");
    }));
    svr.Get(R"(/api/patients/([^/]+)/history)", route(ALLROLES, [](const Request& req, Response& res, User*) {
        Patient* p = sys.findPatient(req.matches[1]);
        if (!p) return fail(res, 404, "Patient not found.");
        vector<string> vs; for (auto v : sys.visitsOf(p->getId())) vs.push_back(visitJ(*v));
        ok(res, Obj().raw("patient", patientJ(*p)).raw("visits", arr(vs)).done());
    }));

    // ----- doctors & services -----
    svr.Get("/api/doctors", route(ALLROLES, [](const Request&, Response& res, User*) {
        vector<string> out; for (auto d : sys.doctors()) out.push_back(userJ(d)); ok(res, arr(out));
    }));
    svr.Get("/api/services", route(ALLROLES, [](const Request&, Response& res, User*) {
        vector<string> out; for (auto& s : sys.allServices()) out.push_back(serviceJ(*s)); ok(res, arr(out));
    }));

    // ----- appointments -----
    svr.Get("/api/appointments", route(ALLROLES, [](const Request&, Response& res, User* u) {
        vector<string> out;
        for (auto& a : sys.allAppointments()) if (u->role() != "Doctor" || a.getDoctorId() == u->getId()) out.push_back(apptJ(a));
        ok(res, arr(out));
    }));
    svr.Post("/api/appointments", route(AR, [](const Request& req, Response& res, User*) {
        Patient* p = sys.findPatient(P(req, "patientId")); Doctor* d = sys.findDoctor(P(req, "doctorId"));
        string date = P(req, "date"), time = P(req, "time"), reason = P(req, "reason");
        if (!p) return fail(res, 400, "Select a patient.");
        if (!d) return fail(res, 400, "Select a doctor.");
        if (!isDate(date)) return fail(res, 400, "Date must be YYYY-MM-DD.");
        if (!isTime(time)) return fail(res, 400, "Time must be HH:MM.");
        if (reason.empty()) return fail(res, 400, "Enter a reason.");
        if (sys.slotTaken(d->getId(), date, time)) return fail(res, 400, "That doctor already has an appointment at that time.");
        ok(res, Obj().str("id", sys.addAppointment(Appointment("", p->getId(), d->getId(), date, time, reason))).done());
    }));
    svr.Post(R"(/api/appointments/([^/]+)/cancel)", route(AR, [](const Request& req, Response& res, User*) {
        Appointment* a = sys.findAppointment(req.matches[1]);
        if (!a) return fail(res, 404, "Appointment not found.");
        if (a->getStatus() != "Scheduled") return fail(res, 400, "Only scheduled appointments can be cancelled.");
        a->setStatus("Cancelled"); sys.save(); ok(res, "{}");
    }));

    // ----- medical visits (Doctor -> Billing integration) -----
    svr.Post("/api/visits", route(DOC, [](const Request& req, Response& res, User* u) {
        Patient* p = sys.findPatient(P(req, "patientId"));
        string diag = P(req, "diagnosis"), notes = P(req, "notes"), apId = P(req, "appointmentId");
        if (!p) return fail(res, 400, "Select a patient.");
        if (diag.empty() || notes.empty()) return fail(res, 400, "Enter diagnosis and medical notes.");
        if (!apId.empty()) {
            Appointment* a = sys.findAppointment(apId);
            if (!a || a->getDoctorId() != u->getId() || a->getPatientId() != p->getId() || a->getStatus() != "Scheduled")
                return fail(res, 400, "Invalid appointment selected.");
        }
        string vid = sys.addVisit(MedicalVisit("", p->getId(), u->getId(), apId, today(), diag, notes));
        for (auto& sid : split(P(req, "services"), ','))
            if (!sid.empty()) sys.addServiceToVisit(vid, sid);   // price looked up + added to bill automatically
        ok(res, Obj().str("visitId", vid).raw("bill", billJ(*sys.billForVisit(vid))).done());
    }));
    svr.Post(R"(/api/visits/([^/]+)/services)", route(DOC, [](const Request& req, Response& res, User* u) {
        MedicalVisit* v = sys.findVisit(req.matches[1]);
        if (!v || v->getDoctorId() != u->getId()) return fail(res, 404, "Visit not found.");
        string err = sys.addServiceToVisit(v->getId(), P(req, "serviceId"));
        if (!err.empty()) return fail(res, 400, err);
        ok(res, billJ(*sys.billForVisit(v->getId())));
    }));

    // ----- billing -----
    svr.Get("/api/bills", route(AR, [](const Request&, Response& res, User*) {
        vector<string> out; for (auto& b : sys.allBills()) out.push_back(billJ(b)); ok(res, arr(out));
    }));
    svr.Post(R"(/api/bills/([^/]+)/paid)", route(AR, [](const Request& req, Response& res, User*) {
        Bill* b = sys.findBill(req.matches[1]);
        if (!b) return fail(res, 404, "Bill not found.");
        b->setPaid(P(req, "paid") == "1"); sys.save(); ok(res, billJ(*b));
    }));
    svr.Post(R"(/api/bills/([^/]+)/other)", route(AR, [](const Request& req, Response& res, User*) {
        Bill* b = sys.findBill(req.matches[1]);
        if (!b) return fail(res, 404, "Bill not found.");
        if (b->isPaid()) return fail(res, 400, "Bill is already paid.");
        int amt = atoi(P(req, "amount").c_str());
        if (amt <= 0) return fail(res, 400, "Enter a valid amount.");
        b->setOtherCharges(b->getOtherCharges() + amt); sys.save(); ok(res, billJ(*b));
    }));

    // ----- staff (Admin) -----
    svr.Get(R"(/api/staff/([^/]+))", route(ADMIN, [](const Request& req, Response& res, User*) {
        string role = req.matches[1];
        if (role != "Doctor" && role != "Receptionist") return fail(res, 400, "Bad role.");
        vector<string> out; for (auto u : sys.usersByRole(role)) out.push_back(userJ(u)); ok(res, arr(out));
    }));
    svr.Post("/api/staff", route(ADMIN, [](const Request& req, Response& res, User*) {
        string role = P(req, "role"), un = P(req, "username"), pw = P(req, "password"), nm = P(req, "name");
        if (role != "Doctor" && role != "Receptionist") return fail(res, 400, "Bad role.");
        if (un.empty() || pw.empty() || nm.empty()) return fail(res, 400, "Please fill in all fields.");
        if (sys.usernameTaken(un)) return fail(res, 400, "Username already exists.");
        if (role == "Doctor") {
            string sp = P(req, "specialization"); int fee = atoi(P(req, "fee").c_str());
            if (sp.empty() || fee <= 0) return fail(res, 400, "Enter specialization and a valid fee.");
            sys.addUser(make_unique<Doctor>(sys.nextUserId('D'), un, pw, nm, sp, fee));
        } else sys.addUser(make_unique<Receptionist>(sys.nextUserId('R'), un, pw, nm));
        ok(res, "{}");
    }));
    svr.Post(R"(/api/staff/([^/]+)/delete)", route(ADMIN, [](const Request& req, Response& res, User*) {
        User* u = sys.findUser(req.matches[1]);
        if (!u || u->role() == "Admin") return fail(res, 404, "Staff member not found.");
        string err = sys.deleteUser(u->getId());
        if (!err.empty()) return fail(res, 400, err);
        ok(res, "{}");
    }));
    svr.Post(R"(/api/staff/([^/]+)/password)", route(ADMIN, [](const Request& req, Response& res, User*) {
        User* u = sys.findUser(req.matches[1]); string pw = P(req, "password");
        if (!u || u->role() == "Admin") return fail(res, 404, "Staff member not found.");
        if (pw.empty()) return fail(res, 400, "Enter a password.");
        u->setPassword(pw); sys.save(); ok(res, "{}");
    }));

    // ----- reports (Admin) -----
    svr.Get("/api/reports", route(ADMIN, [](const Request&, Response& res, User*) {
        vector<string> pats, docs;
        for (auto& p : sys.allPatients())
            pats.push_back(Obj().str("id", p.getId()).str("name", p.getName()).num("visits", sys.visitsOf(p.getId()).size()).done());
        for (auto d : sys.doctors()) {
            int ap = 0, vs = 0;
            for (auto& a : sys.allAppointments()) if (a.getDoctorId() == d->getId()) ap++;
            for (auto& v : sys.allVisits()) if (v.getDoctorId() == d->getId()) vs++;
            docs.push_back(Obj().str("id", d->getId()).str("name", d->getName()).str("specialization", d->getSpecialization())
                .num("appointments", ap).num("visits", vs).done());
        }
        int sc = 0, co = 0, ca = 0, paid = 0, unpaid = 0;
        for (auto& a : sys.allAppointments()) { if (a.getStatus() == "Scheduled") sc++; else if (a.getStatus() == "Completed") co++; else ca++; }
        for (auto& b : sys.allBills()) (b.isPaid() ? paid : unpaid) += b.total();
        ok(res, Obj().raw("patients", arr(pats)).raw("doctors", arr(docs))
            .raw("appointments", Obj().num("scheduled", sc).num("completed", co).num("cancelled", ca).done())
            .raw("billing", Obj().num("count", sys.allBills().size()).num("paid", paid).num("unpaid", unpaid).num("total", paid + unpaid).done())
            .done());
    }));

    // ----- start -----
    int port = 8080;
    while (port < 8100 && !svr.bind_to_port("127.0.0.1", port)) port++;
    if (port >= 8100) { cerr << "Could not open a port.\n"; return 1; }
    string url = "http://localhost:" + to_string(port);
    cout << "Hospital Management System running at " << url << "\nKeep this window open. Press Ctrl+C to stop." << endl;
#ifdef _WIN32
    system(("start " + url).c_str());
#elif __APPLE__
    system(("open " + url).c_str());
#else
    system(("xdg-open " + url + " >/dev/null 2>&1 &").c_str());
#endif
    svr.listen_after_bind();
    return 0;
}
