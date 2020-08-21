#
# TrinketTouringMachine --
#
# Functionality
#
# Mode 0: TuringMachine-like
# - LEDs:  root note color w/ white marching 'step' light, white turns pink w/ random
# - Knob0 - Length Knob: length of sequence from 1-8
# - Knob1 - Random Knob:
#    - >midpoint: sequences are random  (marching white LED is pink)
# - Button:
#    - hold : Random knob sets root note
#    - tap  : change mode
#
# Mode 1: Setup mode
# - LEDs: hue indicates root note
# - Knob0 - Length - sets tempo 60-180 bpm, or EXT when fully clockwise
# - Knob1 - Random - sets scale?
# - Button:
#   - hold : nothing
#   - tap  : change mode
# 
#
# Something like:
# https://musicthing.co.uk/pages/turing.html
#
# 2019-20, @todbot / Tod E. Kurt 
#

import time
import board
import random
import digitalio
import analogio  # from analogio import AnalogOut,AnalogIn
import neopixel
import simpleio  # for map_range()

debug = True

external_trigger = False

class TrinketTouringMachine:
    button_threshold   = 4; # 16 for 10k pot, 4 for 50k pot
    k0_loop_thresh  = 255 - 8
    k0_rand_threshL = 128 - 8
    k0_rand_threshH = 128 + 8
    pixel_count = 8
    min_tempo = 60
    max_tempo = 180

    # possible scales to use
    scales = (
        (0, 2, 3, 5, 7, 8, 10, 12 ),  # minor: Root – WS – HS – WS – WS – HS – WS – WS
        (0, 3, 5, 6, 7, 10 ),         # blues: Root – 3HS – WS – HS – HS – 3HS – WS
        (0, 4, 7, 10),                # dominant seventh
        (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11),  # all notes   
        (0,12),                        # octave
    )


    # Converting notes to DAC values to voltages:
    #
    # 10-bit DAC (1024 values) => 0.0032 Volts per bit @ 3.3v
    # CP represents analogout values as 16-bit (0-65535)
    # e.g. (78/1024)*64 = 4992/65536 = 0.25V
    # 1V in 10-bits => ( 1.0 / 3.3 ) * 1024 = 310.3030 = 
    # 3V in 10-bits => ( 3.0 / 3.3 ) * 1024 = 931
    # => scaled to 16-bits: 310 / 1024 = 19840 / 65536
    #
    # 1V / octave => (1/12) = (0.8333) volt/note =>  ( 39 notes at 3.3V )
    #  (1/12) / ((1/1024) * 3.3)  = 25.858585 / 1024 = (1655 / 65536) per note
    # in 10-bit: 310 / 12 = 25.8 / 1024
    # in 16-bit: 19840 / 12 = 1653.33 / 65536
    #
    # "Writing 5000 is the same as setting it to 5000 / 64 = 78, and 78 / 1024 * 3.3V = 0.25V output."
    # https://learn.adafruit.com/adafruit-trinket-m0-circuitpython-arduino/circuitpython-analog-out-2
    dacval_per_note = 1653.33

    def note_to_dacval(self,noteval):
        dacval = noteval * self.dacval_per_note
        return dacval

    def __init__(self):
        self.pixels = neopixel.NeoPixel( board.D2, self.pixel_count,
                                         brightness = 0.1,
                                         auto_write=False,  # only write when I say to
                                         pixel_order=neopixel.GRB)
        self.clk        = digitalio.DigitalInOut(board.D0)
        self.clk.switch_to_input(pull=digitalio.Pull.DOWN)
        self.analog_out = analogio.AnalogOut(board.D1)
        self.knob0_in   = analogio.AnalogIn(board.D3)
        self.knob1_in   = analogio.AnalogIn(board.D4)  # button is on knob1 too
        self.button_time = 0
        self.button_press = 0  # debounced state
        self.button_held = False
        self.scale_idx = 0
        self.notes = [0] * 8  # max 8 note array
        self.notes_len = 8 #len(self.notes)
        self.note_idx = 0
        self.note_root = 0
        self.mode = 0
        self.clk_external = external_trigger  # false = internal clock
        self.clk_last_value = False
        self.tempo = 120
        self.tempo_secs = 60.0 / self.tempo
        self.last_time = 0
        self.populate_notes()
        
        print("machine is online. clock: {}\n notes:{}".format("ext" if self.clk_external else "int", self.notes ))

    def debug(self):
        note = self.notes[self.note_idx]
        cv = int(self.note_to_dacval(note))
        #if self.note_idx == 0: print("--top--")
        print('t:{} mode:{}{} len:{:2} i:{:2} note:{:2} cv:{:5} k0:{:3} k1:{:3} b:{} bh:{} bt:{}'.
              format(self.tempo, self.mode, "E" if self.clk_external else "I", self.notes_len, self.note_idx, note, cv, 
                     self.knob0, self.knob1, self.button_press, self.button_held, time.monotonic() - self.button_time))

    # read the two knobs and the one button
    # twist is, knob1 is kept above 0V by a resistor and
    # the button pulls knob1's value to 0V if pressed
    #
    def update_knobs_and_button(self):
        self.knob0 = int(self.knob0_in.value/256)
        self.knob1 = int(self.knob1_in.value/256)
        self.button_change = False
        button_newpress = self.knob1 < self.button_threshold  # button pressed when under threshold

        now = time.monotonic()
        button_delta_t = now - self.button_time
        if button_newpress != (self.button_press>0):  # change detected
            self.button_time = now
            if self.button_time == 0:
                pass  #self.button_time = now  # the beginning
            elif button_delta_t > 0.1:  # 100ms minimum hold time
                #print("change w/ debounce!\n")
                self.button_press = button_newpress
                self.button_change = True
            else:
                pass
        else:
            self.button_held =  button_newpress and button_delta_t > 1.0  # button held

                
    #
    #
    #
    def update_leds_mode0(self):
        root_color = neopixel._pixelbuf.wheel( 30 + self.note_root*20 )

        for i in range(self.pixel_count):
            if self.note_idx == i:
                r = 200 * (self.knob0 > 150)
                self.pixels[i] = (255, 255-r, 255-r)  #  current position
            elif i < self.notes_len:
                g = 1 + 40*(self.notes[i] % 6)
                note_color = (0, g, 0)
                if self.notes[i] % 12 == 0:
                    note_color = root_color
                self.pixels[i] = note_color
            else:
                self.pixels[i] = (0,0,0)

        if self.button_held:
            self.pixels[self.pixel_count-1] = root_color
        
        self.pixels.show()

    def update_leds_mode1(self):
        tempo_color = neopixel._pixelbuf.wheel( int(simpleio.map_range(self.tempo, self.min_tempo,self.max_tempo, 0,230 )))
        for i in range(self.pixel_count):
            self.pixels[i] = tempo_color
        self.pixels.show()

    
    def update_leds(self):
        if self.mode == 0:
            self.update_leds_mode0()
        else:
            self.update_leds_mode1()


    def handle_ui(self):
        # change mode on button press & release, but not hold
        if self.button_change and not self.button_held and not self.button_press:
            self.mode = (self.mode + 1) % 2

        if self.mode == 0:
            
            # change root note if button is held
            if self.button_held:
                self.note_root = int(simpleio.map_range(self.knob0, 0,255, 0,11))
            
            # update length based on knob1, but ignore knob1 if button pressedW
            if not self.button_press:
                self.notes_len = int(simpleio.map_range(self.knob1, 0,255, 1,len(self.notes)))

            # turn on random if knob > midpoint
            if self.knob0 > 150:
                #self.populate_notes()  # randomize
                #self.alter_note( self.note_idx, random.random() ) # FIXME
                self.notes[self.note_idx] = self.random_note_in_scale()
            
        elif self.mode == 1:
            if self.knob0 > 254:
                self.clk_external = True
            else:
                self.clk_external = False
                self.tempo = self.min_tempo + simpleio.map_range(self.knob0, 0,255, 0,self.max_tempo-self.min_tempo)  # tempo can range from 60 - 180
                self.tempo_secs = 60.0 / self.tempo
        
        else:
            print("unknown mode")

        
    #
    #
    def process(self):
        self.update_knobs_and_button()
        self.update_leds()
        self.handle_ui()

        triggered = False
        if self.clk_external:  # external clocking
            triggered = self.clk.value and not self.clk_last
            self.clk_last = self.clk.value

        else:             # internal clock
            # check to see if it's time for the next step
            triggered = (time.monotonic() - self.last_time) > self.tempo_secs
            #print("t:{} lt:{} ts:{}".format(triggered,self.last_time,self.tempo_secs))
                  
        if not triggered: return
        
        self.last_time = time.monotonic()  # save the time for next go around

        # get next note, potentially change it randomly, send it out as CV
        note = self.notes[self.note_idx]
        note = self.note_root + note
        self.output_note_as_cv( note )

        self.debug()

        # go to next note
        self.note_idx = (self.note_idx + 1) % self.notes_len


    def populate_notes(self):
#        scale = self.scales[self.scale_idx]
        for i in range(self.notes_len):
            #randi = random.randint(0,len(scale)-1)
            #note = scale[ randi ]
            self.notes[i] = self.random_note_in_scale()
        
 #   def alter_current_note(self):
  #      self.notes[ self.note_idx ]
    
    # alter the given note if rand_amount is above threshold
#    def alter_note(self, idx, rand_amount):
#        if rand_amount < 0.5:
#            self.notes[idx] = self.random_note_in_scale()
           

    def output_cv(self,cv):
        self.analog_out.value = cv
        
    def output_note_as_cv(self,note):
        cv = self.note_to_dacval(note)
        print("cv:",int(cv))
        self.analog_out.value = int(cv)

    def random_note_in_scale(self):
        scale = self.scales[self.scale_idx]
        randi = random.randint(0,len(scale)-1)
        note = scale[ randi ]
        return note
        
#    def random_note_in_scale(self,scale):
#        r = random.randint(0,len(scale)-1)
#        note = self.scale[r]
#        return note
    
    def random_cvs():
        for i in range(16):
            cvs[i] = random.randint(0,65535)

    
        
# possible scales to use
#scales = (
#    (0,12),                   # octave
#    (0, 2, 3, 5, 7, 8, 10 ),  # minor: Root – WS – HS – WS – WS – HS – WS – WS
#    (0, 3, 5, 6, 7, 10 ),     # blues: Root – 3HS – WS – HS – HS – 3HS – WS
#    (0, 4, 7, 10),            # dominant seventh
#    (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11),  # all notes   
#)

# which scale to start at
#scale_id = 2

#tempo = 120        # bpm
#ticks_per_clk = 4  #
#steps_count = 16   #

#possible_lens = [2,3,4,5, 6,8,12,16]


machine = TrinketTouringMachine()

# ---------------------------------
# startup

# little demo of analog out
#print("demoing analog out...\n");
#for aout in range(0, 65535, 64):
#    time.sleep(0.001)
    #analog_out.value = aout;
    #machine.output_cv(
#for aout in range(65535,0, 64):
#    time.sleep(0.001)
    #analog_out.value = aout;

#random_notes()

#for i in range(0,notes_len):
#    print("{:2}: note: {}".format(i,notes[i]) )


    
while True:

    machine.process()
   
    
        
# end
