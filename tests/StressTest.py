import threading
import random
import time
import requests
from datetime import datetime, timedelta

# Arrays expandidos con más datos para generar más reservaciones únicas
guest_names = [
    "Juan García", "Ana López", "Pedro Ruiz", "María Fernández", "Luis Gómez",
    "Carmen Díaz", "José Martínez", "Laura Sánchez", "Miguel Torres", "Paula Romero",
    "Antonio Sánchez", "Beatriz Montoya", "Carlos Herrera", "Diana Velázquez", "Enrique Navarro",
    "Francisca Morales", "Guillermo Castillo", "Helena Jiménez", "Ignacio Reyes", "Julieta Campos",
    "Kevin Vargas", "Lorena Fuentes", "Marcos Leiva", "Natalia Cortés", "Óscar Peña",
    "Patricia Búrgos", "Quique Mendez", "Rosa Acosta", "Sergio Dominguez", "Teresa Vega",
    "Ursulo Rivera", "Verónica Hinojosa", "Waldo Parra", "Xenia Cáceres", "Yasmine Sol"
]

guest_emails = [f"user{i}@example.com" for i in range(35)]
guest_phones = [f"+3461234567{i:02d}" for i in range(35)]
room_numbers = list(range(101, 136))  # 35 habitaciones
room_types = [
    "Doble", "Individual", "Suite", "Triple", "Deluxe",
    "Familiar", "Economy", "Superior", "Premium", "Estándar",
    "Presidential", "Penthouse", "Garden", "Ocean View", "Balcony",
    "Studio", "Loft", "Bungalow", "Villa", "Cottage",
    "Executive", "Business", "Romantic", "Family", "Accessory",
    "Boutique", "Luxury", "Standard", "Comfort", "Classic",
    "Modern", "Traditional", "Eco", "Smart", "Penthouse Elite"
]
number_of_guests = [1, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 3, 2, 1, 4, 3, 2, 1, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 3, 2, 1, 4, 3, 2, 1]

base_date = datetime(2026, 2, 1)
check_in_dates = [(base_date + timedelta(days=i)).strftime("%Y-%m-%d") for i in range(90)]
check_out_dates = [(base_date + timedelta(days=i+1)).strftime("%Y-%m-%d") for i in range(90)]

# Generar 90 valores para cada array (35 habitaciones × 90 fechas = 3150 combinaciones válidas)
number_of_nights = [((i % 5) + 1) for i in range(90)]  # 1-5 noches
price_per_night = [(100.0 + (i % 30) * 5) for i in range(90)]  # $100-$250
payment_methods_options = ["credit_card", "cash", "paypal", "bank_transfer", "crypto"]
payment_methods = [payment_methods_options[i % len(payment_methods_options)] for i in range(90)]
paid_flags = [(i % 3) != 1 for i in range(90)]  # 2/3 pagadas, 1/3 pendientes
reservation_status_options = ["confirmed", "pending", "cancelled"]
reservation_statuses = [reservation_status_options[i % len(reservation_status_options)] for i in range(90)]
special_requests_options = [
    "Late check-in", "Early check-out", "Extra bed", "Sea view", "Breakfast included",
    "No smoking", "High floor", "Quiet room", "Pet friendly", "Airport transfer",
    "Late breakfast", "Extra towels", "Crib needed", "Accessibility equipment", "Late checkout",
    "Room service", "Wake-up call", "City view", "Garden view", "Parking needed",
    "WiFi required", "Work desk", "Gym access", "Pool access", "No noise",
    "Temperature control", "Mini bar", "Safe", "DVD player", "Balcony"
]
special_requests = [special_requests_options[i % len(special_requests_options)] for i in range(90)]
total_prices = [price_per_night[i] * number_of_nights[i] for i in range(90)]
timestamps = [1707427200 + i*86400 for i in range(90)]  # Un timestamp por día


def create_payload():
    """Genera un JSON aleatorio con 3150 combinaciones potenciales de (room, fecha)"""
    guest_idx = random.randint(0, 34)      # 35 guests
    room_idx = random.randint(0, 34)       # 35 rooms
    date_idx = random.randint(0, 89)       # 90 dates
    other_idx = random.randint(0, 89)      # 90 valores para otros campos
    
    payload = {
        "guest_name": guest_names[guest_idx],
        "guest_email": guest_emails[guest_idx],
        "guest_phone": guest_phones[guest_idx],
        "room_number": room_numbers[room_idx],
        "room_type": room_types[room_idx],
        "number_of_guests": number_of_guests[guest_idx],
        "check_in_date": check_in_dates[date_idx],      # La combinación (room, fecha) es la clave
        "check_out_date": check_out_dates[date_idx],
        "number_of_nights": number_of_nights[other_idx],
        "price_per_night": price_per_night[other_idx],
        "total_price": total_prices[other_idx],
        "payment_method": payment_methods[other_idx],
        "paid": paid_flags[other_idx],
        "reservation_status": reservation_statuses[other_idx],
        "special_requests": special_requests[other_idx],
        "created_at": timestamps[other_idx],
        "updated_at": timestamps[other_idx]
    }
    return payload


def send_request():
    url = "http://localhost:8080"
    headers = {"Content-Type": "application/json"}
    payload = create_payload()
    try:
        response = requests.post(url, json=payload, headers=headers, timeout=5)
        print(f"Enviado: {payload['guest_name']} -> {response.status_code}")
    except Exception as e:
        print("Error en la petición:", e)


threads = []
for i in range(10000):
    t = threading.Thread(target=send_request)
    threads.append(t)
    t.start()

for t in threads:
    t.join()

print("Todas las peticiones enviadas")