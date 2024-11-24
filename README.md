# CMRI_Servo_node_V1A
 CMRI single servo with adjustment in JMRI

This feature allows you to modify the servo's closed and thrown position values directly, without needing to disconnect the node. For your convenience, I've set default values for the closed and thrown positions to 85 and 95, respectively.

On the first installation, the system is pre-configured with a midpoint value of 90 degrees. Additionally, the servo's movement range has a predefined maximum and minimum of 130 and 60 degrees, providing a solid starting point for most setups.

To ensure your customized settings are retained, the system uses EEPROM to store the adjusted position values. This means your configuration will persist even after a power cycle or restart.

To fine-tune the servo's behavior for your specific needs, you can use JMRI to modify these stored values. This approach simplifies setup and ensures the servo operates precisely as needed without manual hardware adjustments.

### See Wiki for more details
