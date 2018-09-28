from os.path import join
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

env.Replace(
    UPLOADEROTA=join("$PIOPACKAGES_DIR",
                     "framework-arduinoespressif", "tools", "espota.py"),
    UPLOADCMD='$UPLOADEROTA -i $UPLOAD_PORT -p 8266 -f $SOURCE'
)