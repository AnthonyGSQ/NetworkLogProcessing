import threading
import random
import time
import requests

# Arrays de ejemplo con 10 valores cada uno
guest_names = ["Juan García", "Ana López", "Pedro Ruiz", "María Fernández", "Luis Gómez",
               "Carmen Díaz", "José Martínez", "Laura Sánchez", "Miguel Torres", "Paula Romero"]

guest_emails = [f"user{i}@example.com" for i in range(10)]
guest_phones = [f"+3461234567{i}" for i in range(10)]
room_numbers = list(range(101, 111))
room_types = ["Doble", "Individual", "Suite", "Triple", "Deluxe",
              "Familiar", "Economy", "Superior", "Premium", "Estándar"]
number_of_guests = [1, 2, 3, 4, 2, 1, 3, 2, 4, 1]
check_in_dates = [f"2026-02-{15+i}" for i in range(10)]
check_out_dates = [f"2026-02-{16+i}" for i in range(10)]
number_of_nights = [1, 2, 3, 4, 5, 2, 3, 1, 4, 2]
price_per_night = [100.0, 120.0, 150.0, 200.0, 180.0,
                   130.0, 160.0, 140.0, 170.0, 190.0]
total_prices = [p * n for p, n in zip(price_per_night, number_of_nights)]
payment_methods = ["credit_card", "cash", "paypal", "bank_transfer",
                   "credit_card", "cash", "paypal", "credit_card", "cash", "paypal"]
paid_flags = [True, False, True, True, False, True, False, True, True, False]
reservation_statuses = ["confirmed", "pending", "cancelled", "confirmed",
                        "pending", "confirmed", "cancelled", "confirmed", "pending", "confirmed"]
special_requests = ["Late check-in", "Early check-out", "Extra bed", "Sea view",
                    "Breakfast included", "No smoking", "High floor", "Quiet room", "Pet friendly", "Airport transfer"]
timestamps = [1707427200 + i*1000 for i in range(10)]


def create_payload():
    """Genera un JSON aleatorio con datos de las listas"""
    idx = random.randint(0, 9)
    payload = {
        "guest_name": guest_names[idx],
        "guest_email": guest_emails[idx],
        "guest_phone": guest_phones[idx],
        "room_number": room_numbers[idx],
        "room_type": room_types[idx],
        "number_of_guests": number_of_guests[idx],
        "check_in_date": check_in_dates[idx],
        "check_out_date": check_out_dates[idx],
        "number_of_nights": number_of_nights[idx],
        "price_per_night": price_per_night[idx],
        "total_price": total_prices[idx],
        "payment_method": payment_methods[idx],
        "paid": paid_flags[idx],
        "reservation_status": reservation_statuses[idx],
        "special_requests": special_requests[idx],
        "created_at": timestamps[idx],
        "updated_at": timestamps[idx]
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
for i in range(100):
    t = threading.Thread(target=send_request)
    threads.append(t)
    t.start()

for t in threads:
    t.join()

print("Todas las peticiones enviadas")