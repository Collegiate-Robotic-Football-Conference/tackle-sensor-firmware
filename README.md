# Collegiate Robotic Football Tackle Sensor Firmware

## Cloning the Repo

Open a git bash terminal in the directory of your choosing and call the following command:

`git clone https://github.com/ehunck/rfc_tackle_sensor_firmware`

## Building and Developing in STM32CubeIDE

### Downloads
- Download the STM32CubeIDE available on their [website](https://www.st.com/en/development-tools/stm32cubeide.html).  
- Follow all instructions for the default installation.

### Import the Project
- Launch the CubeIDE application.
- Select a directory to use as your workspace. You can use the default or select a directory of your choosing.  Press Launch. 
- Import the project that you downloaded or cloned into the workspace
- ![import-existing-project](Documentation/images/import_project.png)
- Navigate to the project and import.
- ![import-existing-project-rfc](Documentation/images/import_project_rfc.png)

### Building and Navigating
- The project can be built by clicking the hammer (1).
- The project can be debugged by clicking the bug (2).
- ![navigating-cubeide](Documentation/images/navigating_cubeide.png)
- The device needs to be powered and connected to be debugged or flashed.
- Either a SEGGER Jlink or an STLink can be used to connect to the MCU.
- ![debugger](Documentation/images/debugger.png)

## Flashing Only

- Use a tool like [ST32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) if using an STLink or [JFlashLite](https://www.segger.com/downloads/jlink/) if using a SEGGER JLink to load a pre-built binary onto the STM32.
- The "target" is the STM32G031F8P6.

