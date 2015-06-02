from operator import attrgetter
import platform

import time
import pygame
import xinput
Rect = pygame.Rect


class Struct(dict):
    def __init__(self, **kwargs):
        dict.__init__(self, **kwargs)
        self.__dict__.update(**kwargs)



def stick_center_snap(value, snap=0.2):
    # Feeble attempt to compensate for calibration and loose stick.
    if value >= snap or value <= -snap:
        return value
    else:
        return 0.0



def getJoystick():
    WINDOWS_XBOX_360 = False
    JOYSTICK_NAME = ''
    joysticks = xinput.XInputJoystick.enumerate_devices()
    device_numbers = list(map(attrgetter('device_number'), joysticks))
    joystick = None
    if device_numbers:
        joystick = pygame.joystick.Joystick(device_numbers[0])
        JOYSTICK_NAME = joystick.get_name().upper()
        if 'XBOX 360' in JOYSTICK_NAME:
            WINDOWS_XBOX_360 = True
            joystick = xinput.XInputJoystick(device_numbers[0])
        else:
            # put other logic here for handling platform + device type in the event loop
            joystick.init()

    return joystick, WINDOWS_XBOX_360


BUTTON_A = 0
BUTTON_B = 1
BUTTON_X = 2
BUTTON_Y = 3
BUTTON_LB = 4
BUTTON_RB = 5
BUTTON_BACK = 6
BUTTON_START = 7


class controller_feedback():
    def __init__(self):
        
        self.black = pygame.Color('black')
        self.white = pygame.Color('white')
        self.red = pygame.Color('red')
        self.yellow = pygame.Color('yellow')

        
        # button display
        self.button_a = Struct(rect=Rect(190, 100, 10, 10), value=0, color = pygame.Color('Green'))
        self.button_b = Struct(rect=Rect(205, 85, 10, 10), value=0, color = pygame.Color('Red'))
        self.button_x = Struct(rect=Rect(175, 85, 10, 10), value=0, color = pygame.Color('Blue'))
        self.button_y = Struct(rect=Rect(190, 70, 10, 10), value=0, color = pygame.Color('Yellow'))
        self.button_left_bumper = Struct(rect=Rect(10, 50, 20, 10), value=0, color = self.white)
        self.button_right_bumper = Struct(rect=Rect(200, 50, 20, 10), value=0, color = self.white)
        self.button_back = Struct(rect=Rect(80, 70, 10, 10), value=0, color = self.white)
        self.button_start = Struct(rect=Rect(140, 70, 10, 10), value=0, color = self.white)
        self.button_left_stick = Struct(value=0)
        self.button_right_stick = Struct(value=0)
        self.buttons = (
            self.button_a, self.button_b, self.button_x, self.button_y,
            self.button_left_bumper, self.button_right_bumper,
            self.button_back, self.button_start,
            self.button_left_stick, self.button_right_stick)

        self.draw_buttons = (
            self.button_a, self.button_b, self.button_x, self.button_y,
            self.button_left_bumper, self.button_right_bumper,
            self.button_back, self.button_start)

        
        # stick display
        self.left_stick = Struct(rect=Rect(0, 0, 20, 20), x=0.0, y=0.0)
        self.right_stick = Struct(rect=Rect(0, 0, 20, 20), x=0.0, y=0.0)
        self.left_stick.rect.center = Rect(20, 80, 20, 20).center
        self.right_stick.rect.center = Rect(145, 130, 20, 20).center

        # trigger display
        self.left_trigger = Struct(rect=Rect(10, 10, 20, 40), value=0.0)
        self.right_trigger = Struct(rect=Rect(200, 10, 20, 40), value=0.0)

        self.hat = Struct(x = 75,y = 140,value=(0,0), radius = 10, radius2 = 12)

    def draw_button(self,button, screen):
        rect = button.rect
        value = 0 if button.value else 1
        pygame.draw.rect(screen, button.color, rect, value)

    def draw_stick(self,stick, stick_button, screen):
        ox, oy = origin = stick.rect.center
        radius = stick.rect.h
        x, y = int(round(ox + stick.x * radius)), int(round(oy + stick.y * radius))
        if stick_button.value:
            color = self.yellow
        else:
            color = self.red
        pygame.draw.circle(screen, self.white, origin, radius, 1)
        pygame.draw.circle(screen, color, (x, y), 5, 0)

    def draw_trigger(self, trigger, screen):
        rect = trigger.rect
        pygame.draw.rect(screen, self.white, rect, 1)
        if trigger.value > 0.0:
            r = rect.copy()
            r.h = rect.h * trigger.value
            r.bottom = rect.bottom
            screen.fill(self.white, r)

    def draw_hat(self, hat, screen):
        ox,oy = hat.x, hat.y
        radius = hat.radius
        if hat.value[0] != 0 and hat.value[1] != 0:
            radius = 7
        pygame.draw.circle(screen, self.white, (ox,oy), hat.radius2, 1)
        x, y = int(round(ox + hat.value[0] * radius)), int(round(oy - hat.value[1] * radius))
        pygame.draw.circle(screen, self.red, (x, y), 5, 0)
    
    def draw(self, screen, pos):
        for button in self.draw_buttons:
            self.draw_button(button, screen)
        self.draw_stick(self.left_stick, self.button_left_stick, screen)
        self.draw_stick(self.right_stick, self.button_right_stick, screen)
        self.draw_trigger(self.left_trigger, screen)
        self.draw_trigger(self.right_trigger, screen)
        self.draw_hat(self.hat, screen)

def padd(one, two):
    return (one[0] + two[0], one[1] + two[1])


class OutputClass():
    def __init__(self):
        self.armed = False
        self.font = pygame.font.Font(None, 24)
        self.throttle = 0
        self.throttle2 = 0

        self.forward_min = 130
        self.forward_range = 255 - self.forward_min
        self.reverse_min = 110
        self.reverse_range = self.reverse_min
        
        self.angle = 0
        self.reverse = False
        self.update_period = 1.0/10

        self.time = time.clock()

        self.serial = None
        self.serial_ok = False

        self.line = []
        self.linebuffer = []

        self.ink = None
        
    def reverse_toggle(self):
        self.reverse = not(self.reverse)
        
    def arm_toggle(self):
        self.armed = not(self.armed)

    def connect(self, port):
        
        try:
            self.serial = serial.Serial()
            self.serial.setPort(port - 1)
            self.serial.setBaudrate(115200)
            self.serial.setTimeout(1)
            self.serial.open()
            
            self.serial_ok = True
        except Exception as e:
            print("Serial error!!: {}".format(e))


    def set_angle(self, value):
        self.angle = value

    def set_throttle(self, value):
        self.throttle = value

    def set_throttle2(self, value):
        self.throttle2 = value

    def draw(self, surface, pos):
        if not(self.armed):
            text = self.font.render("Disarmed", 1, (255,255,255))
        else:
            text = self.font.render("Armed", 1, (0,255,0))
        pygame.Surface.blit( surface, text, padd(pos, (20,40)) )

        if not(self.serial_ok):
            text = self.font.render("Disconnected", 1, (255,0,0))
        else:
            text = self.font.render("Connected", 1, (255,255,255))
        pygame.Surface.blit( surface, text, padd(pos, (20,20)) )

        text = self.font.render("Throttle {}".format(self.throttle_byte()), 1, (255,255,255))
        pygame.Surface.blit( surface, text, padd(pos, (20,80)) )


        text = self.font.render("Angle {}".format(self.angle_byte()), 1, (255,255,255))
        pygame.Surface.blit( surface, text, padd(pos, (20,100)) )


    def throttle_byte(self):
        #rev = 0.45
        #if self.reverse: rev = -0.45
        #throttle = (self.throttle+self.throttle2)/2.0
        #return int((0.5 + ((  throttle  *rev) ))*255)
        
        throttle = (self.throttle+self.throttle2)/2.0
        if self.reverse:
            return int( self.reverse_min - self.reverse_range*throttle)
        return int( self.forward_min + self.forward_range*throttle)

    def angle_byte(self):
        return int( (0.5 + (self.angle*0.5))*255 )

    def write(self, string):
        if self.serial_ok:
            self.serial.write(string)

    def read(self):
        if self.serial_ok:
            while self.serial.inWaiting():
                ch = self.serial.read(1)
                och = ord(ch)
                if self.ink == None:
                    if ch == 'M':
                        self.ink = ch
                        self.line = []
                    elif ch == 'I':
                        self.ink = ch
                        self.line = []
                    else:
                        print("String start not recognised")
                elif self.ink == 'M':
                    self.line.append(och)
                    if ch == "\n" or len(self.line) > 100:
                        msg = "message: "
                        for ch2 in self.line:
                            msg += chr(ch2)
                        print(msg)
                        self.ink = None
                elif self.ink == 'I':
                    self.line.append(och)
                    if len(self.line) == (97 + 2 + 1):
                        self.linebuffer.append(self.line)
                        self.ink = None
    
    def send(self):
        stime = time.clock()
        elapsed = stime - self.time

        if elapsed > self.update_period:
            
            self.time = stime
            if self.armed and self.serial_ok:
                self.serial_ok = self.serial.isOpen()
                self.write( "M\x01{}\n".format( chr( self.throttle_byte() ) ) )
                self.write( "M\x02{}\n".format( chr( self.angle_byte() ) ) )

    def line_ready(self):
        return len(self.linebuffer)
    
    def get_line(self):
        line = self.linebuffer.pop()
        dat = line[1:97]
        address = (line[-3] - 1)
        return dat, address
    
    def request_image(self):
        if self.armed and self.serial_ok:
            self.write( "C\n" )
    
    #def request_line(self):
    #    if self.armed and self.serial_ok:
    #        self.write( "G\n" )

    def close(self):
        if self.serial_ok:
            self.serial.close()



class Picture():
    def __init__(self, directory):
        self.res = (128,96)
        self.res2 = (self.res[0]*3, self.res[1]*3)
        self.img = pygame.Surface(self.res)
        self.img.fill((0,0,0))
        self.img2 = pygame.Surface(self.res2)
        self.number = 0
        self.directory = directory

    def give_line(self,line, address):
        if len(line) != 96:
            print("line match error")
            return
        
        pix = address * 48
        x = pix % 128
        y = pix / 128

        for i in xrange(48):
            b = 2*i
            color = self.pix(line[b], line[ b+1 ])
            self.img.set_at((x,y), color)
            
            x += 1
            if x == 128:
                x = 0;
                y += 1

    def pix(self,x,y):
        r = (y >> 3) * 255 / 31
        g = (((y & 7) << 3) | (x >> 5)) * 255 / 63
        b = (x & 31) * 255 / 31
        
        return (r,g,b)

    def draw(self, surface, pos):
        pygame.transform.scale(self.img, self.res2, self.img2)
        surface.blit(self.img2, pos)

    def save(self, name = "capture", ext = "png"):
        pygame.image.save(self.img, "{}/{}{}.{}".format(self.directory, name,self.number, ext)  )
        self.number += 1
        




import serial


pygame.init()
pygame.joystick.init()

troller = controller_feedback()

output = OutputClass()
output.connect(13)

pic = Picture("C:\Users\Lambosaurus\Desktop\captures")

screen = pygame.display.set_mode((640, 480))
screen_rect = screen.get_rect()
clock = pygame.time.Clock()

joystick, WINDOWS_XBOX_360 = getJoystick()
running = True

if not(WINDOWS_XBOX_360):
    print("Controller not found!!")
    running = False


max_fps = 60

has_captured = False


while running:
    clock.tick(max_fps)
    if WINDOWS_XBOX_360:
        joystick.dispatch_events()

    for e in pygame.event.get():
        #print('event: {}'.format(pygame.event.event_name(e.type)))
        if e.type == pygame.JOYAXISMOTION:
            #print('JOYAXISMOTION: axis {}, value {}'.format(e.axis, e.value))
            if e.axis == 2:
                troller.left_trigger.value = e.value
                output.set_throttle2(e.value)
            elif e.axis == 5:
                troller.right_trigger.value = e.value
                output.set_throttle(e.value)
            elif e.axis == 0:
                troller.left_stick.y = stick_center_snap(e.value * -1)
            elif e.axis == 1:
                troller.left_stick.x = stick_center_snap(e.value)
                output.set_angle(troller.left_stick.x)
            elif e.axis == 3:
                troller.right_stick.y = stick_center_snap(e.value * -1)
            elif e.axis == 4:
                troller.right_stick.x = stick_center_snap(e.value)
        elif e.type == pygame.JOYBUTTONDOWN:
            #print('JOYBUTTONDOWN: button {}'.format(e.button))
            troller.buttons[e.button].value = 1

            if e.button == BUTTON_START:
                output.arm_toggle()
            if e.button == BUTTON_X:
                output.reverse_toggle()
            if e.button == BUTTON_Y:
                if has_captured:
                    pic.save()
                output.request_image()
                has_captured = True
            #if e.button == BUTTON_B:
            #    output.request_line()
            
        elif e.type == pygame.JOYBUTTONUP:
            #print('JOYBUTTONUP: button {}'.format(e.button))
            troller.buttons[e.button].value = 0
        elif e.type == pygame.JOYHATMOTION:
            # pygame sends this; xinput sends a button instead--the handler converts the button to a hat event
            #print('JOYHATMOTION: joy {} hat {} value {}'.format(e.joy, e.hat, e.value))
            troller.hat.value = e.value
                
        elif e.type == pygame.KEYDOWN:
            if e.key == pygame.K_ESCAPE:
                running = False
            if e.key == pygame.K_SPACE:
                pic.save()
                has_captured = False
        elif e.type == pygame.QUIT:
            running = False

    # draw the controls
    screen.fill( (0,0,0) )
    troller.draw(screen, (0,0))
    output.draw(screen, (250,0))
    output.send()
    output.read()
    if output.line_ready():
        (line, address) = output.get_line()
        pic.give_line(line,address)
    pic.draw(screen, (50,180))
    pygame.display.flip()

output.close()    
pygame.quit()
