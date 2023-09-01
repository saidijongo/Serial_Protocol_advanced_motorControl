import serial
import threading
import tkinter as tk
from tkinter import ttk

class DaminRobot:
    def __init__(self, root):
        self.root = root
        self.root.title("Robot Head Control")
        self.root.configure(bg="blue")

        self.ser = None
        self.connection_status = None

        self.connect_to_arduino()

        self.create_widgets()

        self.serial_thread = threading.Thread(target=self.read_serial)
        self.serial_thread.daemon = True
        self.serial_thread.start()

    def connect_to_arduino(self):
        try:
            self.ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
            self.update_connection_status("Arduino is connected", "green")
        except serial.SerialException:
            self.update_connection_status("", "red")

    def update_connection_status(self, text, fg_color):
        if self.connection_status:
            self.connection_status.destroy()
        self.connection_status = tk.Label(self.root, text=text, fg=fg_color, bg="yellow")
        self.connection_status.grid(row=0, column=0, columnspan=2, padx=10, pady=10)

    def create_widgets(self):
        self.reset_button = ttk.Button(self.root, text="RESET (R)", command=self.send_reset)
        self.reset_button.grid(row=1, column=0, padx=10, pady=10)

        self.cw_button = ttk.Button(self.root, text="CW 90° (C)", command=self.send_cw)
        self.cw_button.grid(row=1, column=1, padx=10, pady=10)

        self.ccw_button = ttk.Button(self.root, text="CCW 90° (D)", command=self.send_ccw)
        self.ccw_button.grid(row=2, column=0, padx=10, pady=10)

        self.info_button = ttk.Button(self.root, text="Info (I)", command=self.request_info)
        self.info_button.grid(row=2, column=1, padx=10, pady=10)

        self.angle_label = ttk.Label(self.root, text="Enter Angle:")
        self.angle_label.grid(row=3, column=0, padx=10, pady=10)

        self.angle_entry = ttk.Entry(self.root)
        self.angle_entry.grid(row=3, column=1, padx=10, pady=10)

        self.send_angle_button = ttk.Button(self.root, text="Send Angle", command=self.send_angle)
        self.send_angle_button.grid(row=4, column=0, columnspan=2, padx=10, pady=10)

        self.status_label = ttk.Label(self.root, text="Status:")
        self.status_label.grid(row=5, column=0, padx=10, pady=10)

        self.status_var = tk.StringVar()
        self.status_var.set("Waiting for command...")
        self.status_display = ttk.Label(self.root, textvariable=self.status_var)
        self.status_display.grid(row=5, column=1, padx=10, pady=10)

    def send_command(self, command):
        if self.ser:
            self.ser.write(command.encode() + b'\n')
            self.status_var.set(f"Sent command: {command}")
        else:
            self.status_var.set("Arduino Not Connected")

    def send_reset(self):
        self.send_command("R")

    def send_cw(self):
        self.send_command("C")

    def send_ccw(self):
        self.send_command("D")

    def request_info(self):
        self.send_command("I")

    def send_angle(self):
        angle = self.angle_entry.get()
        if angle:
            self.send_command(angle)

    def read_serial(self):
        while True:
            if self.ser:
                data = self.ser.readline().decode().strip()
                if data:
                    print("Received:", data)
                    self.status_var.set(data)
            else:
                print("Arduino Not Connected")

    def close_serial(self):
        if self.ser:
            self.ser.close()

if __name__ == "__main__":
    root = tk.Tk()
    app = DaminRobot(root)
    root.protocol("WM_DELETE_WINDOW", app.close_serial)
    root.mainloop()
