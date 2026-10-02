/* reports.js
   Owner: Sarthak
   Reports page (Admin). */

/* ========== 10. REPORTS (admin) ========== */

async function showReports() {
  const r = await api('reports');
  $('#ptools').innerHTML = '<button class="btn ghost" onclick="window.print()">🖨 Print</button>';

  const summary = stat(r.appointments.scheduled, 'Scheduled')
                + stat(r.appointments.completed, 'Completed')
                + stat(r.appointments.cancelled, 'Cancelled')
                + stat(money(r.billing.paid), 'Collected (paid)')
                + stat(money(r.billing.unpaid), 'Outstanding (unpaid)')
                + stat(money(r.billing.total), `Total billed (${r.billing.count} bills)`);

  const doctorRows = r.doctors.map(d => [d.id, esc(d.name), esc(d.specialization), d.appointments, d.visits]);
  const patientRows = r.patients.map(p => [p.id, esc(p.name), p.visits]);

  setContent(`
    <div class="cards">${summary}</div>
    <h3>Doctor report</h3>
    ${table(['ID', 'Doctor', 'Specialization', 'Appointments', 'Visits'], doctorRows)}
    <h3>Patient report</h3>
    ${patientRows.length ? table(['ID', 'Patient', 'Visits'], patientRows) : emptyBox('No patients.')}`);
}
