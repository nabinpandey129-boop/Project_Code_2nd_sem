/* dashboard.js
   Owner: Sarthak
   Dashboard page. */

/* ========== 4. DASHBOARD ========== */

async function showDashboard() {
  const s = await api('summary');
  const isDoctor = me.role === 'Doctor';

  let cards = stat(s.patients, 'Patients')
            + stat(s.scheduled, isDoctor ? 'My scheduled appointments' : 'Scheduled appointments')
            + stat(s.today, 'Appointments today');
  if (!isDoctor) cards += stat(s.unpaid, 'Unpaid bills');
  if (me.role === 'Admin') cards += stat(s.doctors, 'Doctors');
  cards += stat(s.visits, 'Medical visits');

  setContent(`
    <div class="card">
      <h3 style="margin:0">Welcome, ${esc(me.name)} 👋</h3>
      <p style="color:var(--muted);margin:6px 0 0">Signed in as ${me.role}.</p>
    </div>
    <div class="cards">${cards}</div>`);
}
