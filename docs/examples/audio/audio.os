const soundDevices = AudioAccess.getAvailableSoundDevices();
console.log(soundDevices);
const file = AudioAccess.open("/examples/audio/Minuet in F major .mp3");
file.play();