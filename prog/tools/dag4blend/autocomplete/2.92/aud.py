import sys
import typing
import bpy.types


class Device:
    ''' Device objects represent an audio output backend like OpenAL or SDL, but might also represent a file output or RAM buffer output.
    '''

    channels = None
    ''' The channel count of the device.'''

    distance_model = None
    ''' The distance model of the device.'''

    doppler_factor = None
    ''' The doppler factor of the device. This factor is a scaling factor for the velocity vectors in doppler calculation. So a value bigger than 1 will exaggerate the effect as it raises the velocity.'''

    format = None
    ''' The native sample format of the device.'''

    listener_location = None
    ''' The listeners's location in 3D space, a 3D tuple of floats.'''

    listener_orientation = None
    ''' The listener's orientation in 3D space as quaternion, a 4 float tuple.'''

    listener_velocity = None
    ''' The listener's velocity in 3D space, a 3D tuple of floats.'''

    rate = None
    ''' The sampling rate of the device in Hz.'''

    speed_of_sound = None
    ''' The speed of sound of the device. The speed of sound in air is typically 343.3 m/s.'''

    volume = None
    ''' The overall volume of the device.'''

    @classmethod
    def lock(cls):
        ''' Locks the device so that it's guaranteed, that no samples are read from the streams until :meth: unlock is called. This is useful if you want to do start/stop/pause/resume some sounds at the same time.

        '''
        pass

    @classmethod
    def play(cls, sound: 'bpy.types.Sound', keep: bool = False) -> 'Handle':
        ''' Plays a sound.

        :param sound: The sound to play.
        :type sound: 'bpy.types.Sound'
        :param keep: Handle.keep .
        :type keep: bool
        :rtype: 'Handle'
        :return: The playback handle with which playback can be controlled with.
        '''
        pass

    @classmethod
    def stopAll(cls):
        ''' Stops all playing and paused sounds.

        '''
        pass

    @classmethod
    def unlock(cls):
        ''' Unlocks the device after a lock call, see :meth: lock for details.

        '''
        pass


class DynamicMusic:
    ''' The DynamicMusic object allows to play music depending on a current scene, scene changes are managed by the class, with the possibility of custom transitions. The default transition is a crossfade effect, and the default scene is silent and has id 0
    '''

    fadeTime = None
    ''' The length in seconds of the crossfade transition'''

    position = None
    ''' The playback position of the scene in seconds.'''

    scene = None
    ''' The current scene'''

    status = None
    ''' Whether the scene is playing, paused or stopped (=invalid).'''

    volume = None
    ''' The volume of the scene.'''

    @classmethod
    def addScene(cls, scene: 'bpy.types.Sound') -> int:
        ''' Adds a new scene.

        :param scene: The scene sound.
        :type scene: 'bpy.types.Sound'
        :rtype: int
        :return: The new scene id.
        '''
        pass

    @classmethod
    def addTransition(cls, ini: int, end: int,
                      transition: 'bpy.types.Sound') -> bool:
        ''' Adds a new scene.

        :param ini: the initial scene foor the transition.
        :type ini: int
        :param end: The final scene for the transition.
        :type end: int
        :param transition: The transition sound.
        :type transition: 'bpy.types.Sound'
        :rtype: bool
        :return: false if the ini or end scenes don't exist, true othrwise.
        '''
        pass

    @classmethod
    def pause(cls) -> bool:
        ''' Pauses playback of the scene.

        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def resume(cls) -> bool:
        ''' Resumes playback of the scene.

        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def stop(cls) -> bool:
        ''' Stops playback of the scene.

        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass


class Handle:
    ''' Handle objects are playback handles that can be used to control playback of a sound. If a sound is played back multiple times then there are as many handles.
    '''

    attenuation = None
    ''' This factor is used for distance based attenuation of the source.'''

    cone_angle_inner = None
    ''' The opening angle of the inner cone of the source. If the cone values of a source are set there are two (audible) cones with the apex at the :attr: location of the source and with infinite height, heading in the direction of the source's :attr: orientation . In the inner cone the volume is normal. Outside the outer cone the volume will be :attr: cone_volume_outer and in the area between the volume will be interpolated linearly.'''

    cone_angle_outer = None
    ''' The opening angle of the outer cone of the source.'''

    cone_volume_outer = None
    ''' The volume outside the outer cone of the source.'''

    distance_maximum = None
    ''' The maximum distance of the source. If the listener is further away the source volume will be 0.'''

    distance_reference = None
    ''' The reference distance of the source. At this distance the volume will be exactly :attr: volume .'''

    keep = None
    ''' Whether the sound should be kept paused in the device when its end is reached. This can be used to seek the sound to some position and start playback again.'''

    location = None
    ''' The source's location in 3D space, a 3D tuple of floats.'''

    loop_count = None
    ''' The (remaining) loop count of the sound. A negative value indicates infinity.'''

    orientation = None
    ''' The source's orientation in 3D space as quaternion, a 4 float tuple.'''

    pitch = None
    ''' The pitch of the sound.'''

    position = None
    ''' The playback position of the sound in seconds.'''

    relative = None
    ''' Whether the source's location, velocity and orientation is relative or absolute to the listener.'''

    status = None
    ''' Whether the sound is playing, paused or stopped (=invalid).'''

    velocity = None
    ''' The source's velocity in 3D space, a 3D tuple of floats.'''

    volume = None
    ''' The volume of the sound.'''

    volume_maximum = None
    ''' The maximum volume of the source.'''

    volume_minimum = None
    ''' The minimum volume of the source.'''

    @classmethod
    def pause(cls) -> bool:
        ''' Pauses playback.

        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def resume(cls) -> bool:
        ''' Resumes playback.

        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def stop(cls) -> bool:
        ''' Stops playback.

        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass


class PlaybackManager:
    ''' A PlabackManager object allows to easily control groups os sounds organized in categories.
    '''

    @classmethod
    def addCategory(cls, volume: float) -> int:
        ''' Adds a category with a custom volume.

        :param volume: The volume for ther new category.
        :type volume: float
        :rtype: int
        :return: The key of the new category.
        '''
        pass

    @classmethod
    def clean(cls):
        ''' Cleans all the invalid and finished sound from the playback manager.

        '''
        pass

    @classmethod
    def getVolume(cls, catKey: int) -> float:
        ''' Retrieves the volume of a category.

        :param catKey: the key of the category.
        :type catKey: int
        :rtype: float
        :return: The volume of the cateogry.
        '''
        pass

    @classmethod
    def pause(cls, catKey: int) -> bool:
        ''' Pauses playback of the category.

        :param catKey: the key of the category.
        :type catKey: int
        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def play(cls, sound: 'bpy.types.Sound', catKey: int) -> 'Handle':
        ''' Plays a sound through the playback manager and assigns it to a category.

        :param sound: The sound to play.
        :type sound: 'bpy.types.Sound'
        :param catKey: the key of the category in which the sound will be added, if it doesn't exist, a new one will be created.
        :type catKey: int
        :rtype: 'Handle'
        :return: The playback handle with which playback can be controlled with.
        '''
        pass

    @classmethod
    def resume(cls, catKey: int) -> bool:
        ''' Resumes playback of the catgory.

        :param catKey: the key of the category.
        :type catKey: int
        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def setVolume(cls, volume: float, catKey: int) -> int:
        ''' Changes the volume of a category.

        :param volume: the new volume value.
        :type volume: float
        :param catKey: the key of the category.
        :type catKey: int
        :rtype: int
        :return: Whether the action succeeded.
        '''
        pass

    @classmethod
    def stop(cls, catKey: int) -> bool:
        ''' Stops playback of the category.

        :param catKey: the key of the category.
        :type catKey: int
        :rtype: bool
        :return: Whether the action succeeded.
        '''
        pass


class Sequence:
    ''' This sound represents sequenced entries to play a sound sequence.
    '''

    channels = None
    ''' The channel count of the sequence.'''

    distance_model = None
    ''' The distance model of the sequence.'''

    doppler_factor = None
    ''' The doppler factor of the sequence. This factor is a scaling factor for the velocity vectors in doppler calculation. So a value bigger than 1 will exaggerate the effect as it raises the velocity.'''

    fps = None
    ''' The listeners's location in 3D space, a 3D tuple of floats.'''

    muted = None
    ''' Whether the whole sequence is muted.'''

    rate = None
    ''' The sampling rate of the sequence in Hz.'''

    speed_of_sound = None
    ''' The speed of sound of the sequence. The speed of sound in air is typically 343.3 m/s.'''

    @classmethod
    def add(cls) -> 'SequenceEntry':
        ''' Adds a new entry to the sequence.

        :param sound: The sound this entry should play.
        :type sound: 'bpy.types.Sound'
        :param begin: The start time.
        :type begin: 
        :param end: The end time or a negative value if determined by the sound.
        :type end: 
        :param skip: How much seconds should be skipped at the beginning.
        :type skip: 
        :rtype: 'SequenceEntry'
        :return: The entry added.
        '''
        pass

    @classmethod
    def remove(cls):
        ''' Removes an entry from the sequence.

        :param entry: The entry to remove.
        :type entry: 'SequenceEntry'
        '''
        pass

    @classmethod
    def setAnimationData(cls):
        ''' Writes animation data to a sequence.

        :param type: The type of animation data.
        :type type: int
        :param frame: The frame this data is for.
        :type frame: int
        :param data: The data to write.
        :type data: typing.List[float]
        :param animated: Whether the attribute is animated.
        :type animated: bool
        '''
        pass


class SequenceEntry:
    ''' SequenceEntry objects represent an entry of a sequenced sound.
    '''

    attenuation = None
    ''' This factor is used for distance based attenuation of the source.'''

    cone_angle_inner = None
    ''' The opening angle of the inner cone of the source. If the cone values of a source are set there are two (audible) cones with the apex at the :attr: location of the source and with infinite height, heading in the direction of the source's :attr: orientation . In the inner cone the volume is normal. Outside the outer cone the volume will be :attr: cone_volume_outer and in the area between the volume will be interpolated linearly.'''

    cone_angle_outer = None
    ''' The opening angle of the outer cone of the source.'''

    cone_volume_outer = None
    ''' The volume outside the outer cone of the source.'''

    distance_maximum = None
    ''' The maximum distance of the source. If the listener is further away the source volume will be 0.'''

    distance_reference = None
    ''' The reference distance of the source. At this distance the volume will be exactly :attr: volume .'''

    muted = None
    ''' Whether the entry is muted.'''

    relative = None
    ''' Whether the source's location, velocity and orientation is relative or absolute to the listener.'''

    sound = None
    ''' The sound the entry is representing and will be played in the sequence.'''

    volume_maximum = None
    ''' The maximum volume of the source.'''

    volume_minimum = None
    ''' The minimum volume of the source.'''

    @classmethod
    def move(cls):
        ''' Moves the entry.

        :param begin: The new start time.
        :type begin: 
        :param end: The new end time or a negative value if unknown.
        :type end: 
        :param skip: How many seconds to skip at the beginning.
        :type skip: 
        '''
        pass

    @classmethod
    def setAnimationData(cls):
        ''' Writes animation data to a sequenced entry.

        :param type: The type of animation data.
        :type type: int
        :param frame: The frame this data is for.
        :type frame: int
        :param data: The data to write.
        :type data: typing.List[float]
        :param animated: Whether the attribute is animated.
        :type animated: bool
        '''
        pass


class Sound:
    ''' Sound objects are immutable and represent a sound that can be played simultaneously multiple times. They are called factories because they create reader objects internally that are used for playback.
    '''

    length = None
    ''' The sample specification of the sound as a tuple with rate and channel count.'''

    specs = None
    ''' The sample specification of the sound as a tuple with rate and channel count.'''

    @classmethod
    def buffer(cls, data, rate) -> 'bpy.types.Sound':
        ''' Creates a sound from a data buffer.

        :param data: The data as two dimensional numpy array.
        :type data: 
        :param rate: The sample rate.
        :type rate: 
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def file(cls, filename: str) -> 'bpy.types.Sound':
        ''' Creates a sound object of a sound file.

        :param filename: Path of the file.
        :type filename: str
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def list(cls) -> 'bpy.types.Sound':
        ''' Creates an empty sound list that can contain several sounds.

        :param random: whether the playback will be random or not.
        :type random: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def sawtooth(cls, frequency: float,
                 rate: int = 48000) -> 'bpy.types.Sound':
        ''' Creates a sawtooth sound which plays a sawtooth wave.

        :param frequency: The frequency of the sawtooth wave in Hz.
        :type frequency: float
        :param rate: The sampling rate in Hz. It's recommended to set this value to the playback device's samling rate to avoid resamping.
        :type rate: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def silence(cls, rate: int = 48000) -> 'bpy.types.Sound':
        ''' Creates a silence sound which plays simple silence.

        :param rate: The sampling rate in Hz. It's recommended to set this value to the playback device's samling rate to avoid resamping.
        :type rate: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def sine(cls, frequency: float, rate: int = 48000) -> 'bpy.types.Sound':
        ''' Creates a sine sound which plays a sine wave.

        :param frequency: The frequency of the sine wave in Hz.
        :type frequency: float
        :param rate: The sampling rate in Hz. It's recommended to set this value to the playback device's samling rate to avoid resamping.
        :type rate: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def square(cls, frequency: float, rate: int = 48000) -> 'bpy.types.Sound':
        ''' Creates a square sound which plays a square wave.

        :param frequency: The frequency of the square wave in Hz.
        :type frequency: float
        :param rate: The sampling rate in Hz. It's recommended to set this value to the playback device's samling rate to avoid resamping.
        :type rate: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def triangle(cls, frequency: float,
                 rate: int = 48000) -> 'bpy.types.Sound':
        ''' Creates a triangle sound which plays a triangle wave.

        :param frequency: The frequency of the triangle wave in Hz.
        :type frequency: float
        :param rate: The sampling rate in Hz. It's recommended to set this value to the playback device's samling rate to avoid resamping.
        :type rate: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def ADSR(cls, attack: float, decay: float, sustain: float,
             release: float) -> 'bpy.types.Sound':
        ''' Attack-Decay-Sustain-Release envelopes the volume of a sound. Note: there is currently no way to trigger the release with this API.

        :param attack: The attack time in seconds.
        :type attack: float
        :param decay: The decay time in seconds.
        :type decay: float
        :param sustain: The sustain level.
        :type sustain: float
        :param release: The release level.
        :type release: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def accumulate(cls, additive=False) -> 'bpy.types.Sound':
        ''' Accumulates a sound by summing over positive input differences thus generating a monotonic sigal. If additivity is set to true negative input differences get added too, but positive ones with a factor of two. Note that with additivity the signal is not monotonic anymore.

        :param time: 
        :type time: bool
        :param additive: 
        :type additive: 
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def addSound(cls, sound: 'bpy.types.Sound'):
        ''' Adds a new sound to a sound list.

        :param sound: The sound that will be added to the list.
        :type sound: 'bpy.types.Sound'
        '''
        pass

    @classmethod
    def cache(cls) -> 'bpy.types.Sound':
        ''' Caches a sound into RAM. This saves CPU usage needed for decoding and file access if the underlying sound reads from a file on the harddisk, but it consumes a lot of memory.

        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def data(cls):
        ''' Retrieves the data of the sound as numpy array.

        :return: A two dimensional numpy float array.
        '''
        pass

    @classmethod
    def delay(cls, time: float) -> 'bpy.types.Sound':
        ''' Delays by playing adding silence in front of the other sound's data.

        :param time: How many seconds of silence should be added before the sound.
        :type time: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def envelope(cls, attack: float, release: float, threshold: float,
                 arthreshold: float) -> 'bpy.types.Sound':
        ''' Delays by playing adding silence in front of the other sound's data.

        :param attack: The attack factor.
        :type attack: float
        :param release: The release factor.
        :type release: float
        :param threshold: The general threshold value.
        :type threshold: float
        :param arthreshold: The attack/release threshold value.
        :type arthreshold: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def fadein(cls, start: float, length: float) -> 'bpy.types.Sound':
        ''' Fades a sound in by raising the volume linearly in the given time interval.

        :param start: Time in seconds when the fading should start.
        :type start: float
        :param length: Time in seconds how long the fading should last.
        :type length: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def fadeout(cls, start: float, length: float) -> 'bpy.types.Sound':
        ''' Fades a sound in by lowering the volume linearly in the given time interval.

        :param start: Time in seconds when the fading should start.
        :type start: float
        :param length: Time in seconds how long the fading should last.
        :type length: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def filter(cls, b: typing.List[float], a=(1)) -> 'bpy.types.Sound':
        ''' Filters a sound with the supplied IIR filter coefficients. Without the second parameter you'll get a FIR filter. If the first value of the a sequence is 0, it will be set to 1 automatically. If the first value of the a sequence is neither 0 nor 1, all filter coefficients will be scaled by this value so that it is 1 in the end, you don't have to scale yourself.

        :param b: The nominator filter coefficients.
        :type b: typing.List[float]
        :param a: The denominator filter coefficients.
        :type a: typing.List[float]
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def highpass(cls, frequency: float, Q: float = 0.5) -> 'bpy.types.Sound':
        ''' Creates a second order highpass filter based on the transfer function :math: H(s) = s^2 / (s^2 + s/Q + 1)

        :param frequency: The cut off trequency of the highpass.
        :type frequency: float
        :param Q: Q factor of the lowpass.
        :type Q: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def join(cls, sound: 'bpy.types.Sound') -> 'bpy.types.Sound':
        ''' Plays two factories in sequence.

        :param sound: The sound to play second.
        :type sound: 'bpy.types.Sound'
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def limit(cls, start: float, end: float) -> 'bpy.types.Sound':
        ''' Limits a sound within a specific start and end time.

        :param start: Start time in seconds.
        :type start: float
        :param end: End time in seconds.
        :type end: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def loop(cls, count) -> 'bpy.types.Sound':
        ''' Loops a sound.

        :param count: How often the sound should be looped. Negative values mean endlessly.
        :type count: 
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def lowpass(cls, frequency: float, Q: float = 0.5) -> 'bpy.types.Sound':
        ''' Creates a second order lowpass filter based on the transfer function :math: H(s) = 1 / (s^2 + s/Q + 1)

        :param frequency: The cut off trequency of the lowpass.
        :type frequency: float
        :param Q: Q factor of the lowpass.
        :type Q: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def mix(cls, sound: 'bpy.types.Sound') -> 'bpy.types.Sound':
        ''' Mixes two factories.

        :param sound: The sound to mix over the other.
        :type sound: 'bpy.types.Sound'
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def modulate(cls, sound: 'bpy.types.Sound') -> 'bpy.types.Sound':
        ''' Modulates two factories.

        :param sound: The sound to modulate over the other.
        :type sound: 'bpy.types.Sound'
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def mutable(cls) -> 'bpy.types.Sound':
        ''' Creates a sound that will be restarted when sought backwards. If the original sound is a sound list, the playing sound can change.

        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def pingpong(cls) -> 'bpy.types.Sound':
        ''' Plays a sound forward and then backward. This is like joining a sound with its reverse.

        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def pitch(cls, factor: float) -> 'bpy.types.Sound':
        ''' Changes the pitch of a sound with a specific factor.

        :param factor: The factor to change the pitch with.
        :type factor: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def rechannel(cls, channels: int) -> 'bpy.types.Sound':
        ''' Rechannels the sound.

        :param channels: The new channel configuration.
        :type channels: int
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def resample(cls, rate, high_quality: bool) -> 'bpy.types.Sound':
        ''' Resamples the sound.

        :param rate: The new sample rate.
        :type rate: 
        :param high_quality: When true use a higher quality but slower resampler.
        :type high_quality: bool
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def reverse(cls) -> 'bpy.types.Sound':
        ''' Plays a sound reversed.

        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def sum(cls) -> 'bpy.types.Sound':
        ''' Sums the samples of a sound.

        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def threshold(cls, threshold=' 0') -> 'bpy.types.Sound':
        ''' Makes a threshold wave out of an audio wave by setting all samples with a amplitude >= threshold to 1, all <= -threshold to -1 and all between to 0.

        :param threshold: Threshold value over which an amplitude counts non-zero.
        :type threshold: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def volume(cls, volume: float) -> 'bpy.types.Sound':
        ''' Changes the volume of a sound.

        :param volume: The new volume..
        :type volume: float
        :rtype: 'bpy.types.Sound'
        :return: Sound object.
        '''
        pass

    @classmethod
    def write(cls, filename: str, rate: int, channels: int, format: int,
              container: int, codec: int, bitrate: int, buffersize: int):
        ''' Writes the sound to a file.

        :param filename: The path to write to.
        :type filename: str
        :param rate: The sample rate to write with.
        :type rate: int
        :param channels: The number of channels to write with.
        :type channels: int
        :param format: The sample format to write with.
        :type format: int
        :param container: The container format for the file.
        :type container: int
        :param codec: The codec to use in the file.
        :type codec: int
        :param bitrate: The bitrate to write with.
        :type bitrate: int
        :param buffersize: The size of the writing buffer.
        :type buffersize: int
        '''
        pass


class Source:
    ''' The source object represents the source position of a binaural sound.
    '''

    azimuth = None
    ''' The azimuth angle.'''

    distance = None
    ''' The distance value. 0 is min, 1 is max.'''

    elevation = None
    ''' The elevation angle.'''


class ThreadPool:
    ''' A ThreadPool is used to parallelize convolution efficiently.
    '''

    pass


class error:
    pass


AP_LOCATION = None
''' constant value 3
'''

AP_ORIENTATION = None
''' constant value 4
'''

AP_PANNING = None
''' constant value 1
'''

AP_PITCH = None
''' constant value 2
'''

AP_VOLUME = None
''' constant value 0
'''

CHANNELS_INVALID = None
''' constant value 0
'''

CHANNELS_MONO = None
''' constant value 1
'''

CHANNELS_STEREO = None
''' constant value 2
'''

CHANNELS_STEREO_LFE = None
''' constant value 3
'''

CHANNELS_SURROUND4 = None
''' constant value 4
'''

CHANNELS_SURROUND5 = None
''' constant value 5
'''

CHANNELS_SURROUND51 = None
''' constant value 6
'''

CHANNELS_SURROUND61 = None
''' constant value 7
'''

CHANNELS_SURROUND71 = None
''' constant value 8
'''

CODEC_AAC = None
''' constant value 1
'''

CODEC_AC3 = None
''' constant value 2
'''

CODEC_FLAC = None
''' constant value 3
'''

CODEC_INVALID = None
''' constant value 0
'''

CODEC_MP2 = None
''' constant value 4
'''

CODEC_MP3 = None
''' constant value 5
'''

CODEC_OPUS = None
''' constant value 8
'''

CODEC_PCM = None
''' constant value 6
'''

CODEC_VORBIS = None
''' constant value 7
'''

CONTAINER_AC3 = None
''' constant value 1
'''

CONTAINER_FLAC = None
''' constant value 2
'''

CONTAINER_INVALID = None
''' constant value 0
'''

CONTAINER_MATROSKA = None
''' constant value 3
'''

CONTAINER_MP2 = None
''' constant value 4
'''

CONTAINER_MP3 = None
''' constant value 5
'''

CONTAINER_OGG = None
''' constant value 6
'''

CONTAINER_WAV = None
''' constant value 7
'''

DISTANCE_MODEL_EXPONENT = None
''' constant value 5
'''

DISTANCE_MODEL_EXPONENT_CLAMPED = None
''' constant value 6
'''

DISTANCE_MODEL_INVALID = None
''' constant value 0
'''

DISTANCE_MODEL_INVERSE = None
''' constant value 1
'''

DISTANCE_MODEL_INVERSE_CLAMPED = None
''' constant value 2
'''

DISTANCE_MODEL_LINEAR = None
''' constant value 3
'''

DISTANCE_MODEL_LINEAR_CLAMPED = None
''' constant value 4
'''

FORMAT_FLOAT32 = None
''' constant value 36
'''

FORMAT_FLOAT64 = None
''' constant value 40
'''

FORMAT_INVALID = None
''' constant value 0
'''

FORMAT_S16 = None
''' constant value 18
'''

FORMAT_S24 = None
''' constant value 19
'''

FORMAT_S32 = None
''' constant value 20
'''

FORMAT_U8 = None
''' constant value 1
'''

RATE_11025 = None
''' constant value 11025
'''

RATE_16000 = None
''' constant value 16000
'''

RATE_192000 = None
''' constant value 192000
'''

RATE_22050 = None
''' constant value 22050
'''

RATE_32000 = None
''' constant value 32000
'''

RATE_44100 = None
''' constant value 44100
'''

RATE_48000 = None
''' constant value 48000
'''

RATE_8000 = None
''' constant value 8000
'''

RATE_88200 = None
''' constant value 88200
'''

RATE_96000 = None
''' constant value 96000
'''

RATE_INVALID = None
''' constant value 0
'''

STATUS_INVALID = None
''' constant value 0
'''

STATUS_PAUSED = None
''' constant value 2
'''

STATUS_PLAYING = None
''' constant value 1
'''

STATUS_STOPPED = None
''' constant value 3
'''
