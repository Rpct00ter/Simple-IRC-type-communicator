mport tkinter as tk
from tkinter import scrolledtext
import socket
import threading

def receive_messages():
    while True:
        try:
            data = client_socket.recv(1024).decode('utf-8')
            if data == "exit":
                break
            message_box.insert(tk.END, f"\n{data}")
        except Exception as e:
            print("Error reading from server:", e)
            break

def send_message():
    message = input_entry.get()
    if message == 'exit':
        client_socket.sendall(message.encode('utf-8'))
        root.destroy()
    try:
        client_socket.sendall(message.encode('utf-8'))
    except Exception as e:
        print("Error writing to server:", e)

# Set up the GUI
root = tk.Tk()
root.title("Simple Chat Client")

# Create and pack widgets
message_box = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=40, height=15)
message_box.pack(padx=10, pady=10)

input_entry = tk.Entry(root, width=30)
input_entry.pack(padx=10, pady=10)

send_button = tk.Button(root, text="Send", command=send_message)
send_button.pack(pady=10)

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
print("Your messages will not be send until you join the room.")
print("Here are the options you can type:")
print("show_users -> shows all logged users")
print("create_room name -> creates a room of the given name")
print("join_room name -> joins you to the chosen room")
print("show_all_rooms -> shows all rooms")
print("show_room_members -> shows all current room members")
print("leave_room -> leave the current room")
print("exit -> exit the app")

# Create threads for sending and receiving messages
receive_thread = threading.Thread(target=receive_messages)
receive_thread.start()

# Start the Tkinter main loop
root.mainloop()

# Wait for the receive thread to finish
receive_thread.join()

# Close the socket when done
client_socket.close()
