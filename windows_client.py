import socket
import threading

# Function to receive messages from the server
def receive_messages():
    while True:
        try:
            data = client_socket.recv(1024).decode('utf-8')
            if data == "exit":
                break
            print("\n", data)
        except Exception as e:
            print("Error reading from server:", e)
            break

# Function to send messages to the server
def send_messages():
    while True:
        message = input("\nEnter message to send (or 'exit' to quit): ")
        if message == 'exit':
            break
        try:
            client_socket.sendall(message.encode('utf-8'))
        except Exception as e:
            print("Error writing to server:", e)
            break

# Get user input for username
username = input("Please provide your nick (one word only):\n")

# Create a socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Set up the server address
host = input("Enter server hostname: ")
port = int(input("Enter server port: "))

# Connect to the server
try:
    client_socket.connect((host, port))
except Exception as e:
    print("Error connecting to server:", e)
    client_socket.close()
    exit(1)

# Send the username to the server
try:
    client_socket.sendall(username.encode('utf-8'))
except Exception as e:
    print("Error sending username to server:", e)
    client_socket.close()
    exit(1)

print(f"Hello {username} :)")
print("You will currently write to yourself until you join the room.")
print("Here are the options you can type:")
print("show_users -> shows all logged users")
print("create_room name -> creates a room of the given name")
print("join_room name -> joins you to the chosen room")
print("show_all_rooms -> shows all rooms")
print("show_room_members -> shows all current room members")

# Create threads for sending and receiving messages
receive_thread = threading.Thread(target=receive_messages)
send_thread = threading.Thread(target=send_messages)

# Start the threads
receive_thread.start()
send_thread.start()

# Wait for threads to finish
send_thread.join()
receive_thread.join()

# Close the socket when done
client_socket.close()

