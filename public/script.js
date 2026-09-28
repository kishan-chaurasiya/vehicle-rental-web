let vehicles=[];
async function loadVehicles(){
  const res=await fetch('/api/vehicles'); vehicles=await res.json();
  const search=document.getElementById('search').value.toLowerCase();
  const type=document.getElementById('typeFilter').value;
  const list=vehicles.filter(v=>(v.name.toLowerCase().includes(search)||v.type.toLowerCase().includes(search))&&(!type||v.type===type));
  const grid=document.getElementById('vehicleGrid');
  const select=document.getElementById('vehicleSelect');
  grid.innerHTML=list.map(v=>`
    <div class="vehicle">
      <div class="vehicle-img">${v.image}</div>
      <div class="vehicle-body">
        <h3>${v.name}</h3><div class="meta">${v.type} • ${v.seats} seats • ${v.fuel} • ${v.transmission}</div>
        <div class="price">₹${v.price} <small>/ day</small></div>
        <button class="primary" onclick="selectVehicle(${v.id})">Book Now</button>
      </div>
    </div>`).join('');
  select.innerHTML='<option value="">Select vehicle</option>'+vehicles.map(v=>`<option value="${v.id}">${v.name} - ₹${v.price}/day</option>`).join('');
}
function selectVehicle(id){document.getElementById('vehicleSelect').value=id;location.hash='book';}
function openLogin(){document.getElementById('loginModal').style.display='flex'}
function closeLogin(){document.getElementById('loginModal').style.display='none'}
async function registerUser(){
  const body=new URLSearchParams({username:regUser.value,password:regPass.value});
  const r=await fetch('/api/register',{method:'POST',body}); const d=await r.json();
  regMsg.textContent=d.message;
}
async function login(){
  const body=new URLSearchParams({username:loginUser.value,password:loginPass.value});
  const r=await fetch('/api/login',{method:'POST',body}); const d=await r.json();
  loginMsg.textContent=d.message;
  if(d.ok){localStorage.setItem('driveeaseUser',loginUser.value);username.value=loginUser.value;closeLogin();loadBookings()}
}
document.getElementById('bookingForm').addEventListener('submit',async e=>{
 e.preventDefault();
 const id=vehicleSelect.value, v=vehicles.find(x=>x.id==id);
 if(!v){alert('Please select a vehicle');return}
 const user=username.value||localStorage.getItem('driveeaseUser');
 if(!user){alert('Please login first');openLogin();return}
 const total=Number(v.price)*Number(days.value);
 const body=new URLSearchParams({username:user,vehicle:v.name,pickup:pickup.value,dropoff:dropoff.value,days:days.value,total:'₹'+total});
 const r=await fetch('/api/book',{method:'POST',body});const d=await r.json();alert(d.message);if(d.ok)loadBookings();
});
async function loadBookings(){
 const user=localStorage.getItem('driveeaseUser'); if(!user)return;
 const r=await fetch('/api/bookings?username='+encodeURIComponent(user)); const data=await r.json();
 bookingList.innerHTML=data.length?data.map(b=>`<div class="booking-item"><strong>${b.vehicle}</strong><br>Pickup: ${b.pickup} &nbsp; Drop-off: ${b.dropoff}<br>Days: ${b.days} &nbsp; <b>Total: ${b.total}</b></div>`).join(''):'<p>No bookings yet.</p>';
}
document.getElementById('search').addEventListener('input',loadVehicles);
document.getElementById('typeFilter').addEventListener('change',loadVehicles);
loadVehicles();loadBookings();
