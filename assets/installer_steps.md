### **Step 1: The Live Environment (Interactive TUI)**

1.  **Auto-Launch:** Launch the ncurses TUI Welcome screen with all steps.
2.  **Environment Check:** Verify network connectivity and scan available disks (`lsblk`).
3.  **CSV Parsing:** Read `matrix.csv` to identify available OS editions and app lists.
4.  **Edition Selection:** Present a menu to choose the target edition (e.g., IoT, SCADA).
5.  **Data Collection:** Input system hostname, SSH public keys, and target disk.
6.  **Ignition Generation:** Merge user inputs and CSV app data into a `config.ign` file.
7.  **Disk Imaging:** Execute `coreos-installer` with the generated Ignition file.
8.  **Hand-off:** Prompt the user to remove the installation media and reboot.

---

### **Step 2: The Target System (Automated Execution)**

1.  **Ignition Initialization:** Ignition runs in `initramfs` to create users and write dotfiles/bins.
2.  **First Boot Service:** A "one-shot" systemd unit (defined in the Ignition file) starts.
3.  **RPM Layering:** The service executes `rpm-ostree install` for the apps listed in your matrix.
4.  **Configuration Check:** Verify that all injected bins and dotfiles have correct permissions.
5.  **Final Cleanup:** Flag the installation as complete to prevent the script from running again.
6.  **Final Reboot:** Perform an automated reboot to switch into the newly layered deployment.
7.  **Production Start:** The system boots into the fully configured **Barbarous** environment.