# Python 3.3.3 and 2.7.6
# python helloworld_python.py

from threading import Thread

i = 0

def adder(step):
# In Python you "import" a global variable, instead of "export"ing it when you declare it
    global i
# In Python 2 this generates a list of integers (which takes time),
# while in Python 3 this is an iterable (which is much faster to generate).
# In python 2, an iterable is created with xrange()
    for x in range(0, 1000000):     
        i += int(step)

def main():
    adder_thr = Thread(target = adder, args=(1,))
    subst_thr = Thread(target = adder, args=(-1,))
    adder_thr.start()
    subst_thr.start()
    adder_thr.join()
    subst_thr.join()
    print(str(i))

main()
