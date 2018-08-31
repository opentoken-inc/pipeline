import signalr
from signalr.events import EventHook
from signalr.hubs._hub import HubServer, HubClient


class HubServerFixed(HubServer):

  def invoke(self, method, *data):
    I = self._HubServer__connection.increment_send_counter()
    self._HubServer__connection.send({
        'H': self.name,
        'M': method,
        'A': data,
        'I': I,
    })
    return I


class NullThread:

  def join(self):
    pass


class HubFixed:

  def __init__(self, name, connection):
    self.name = name
    self.server = HubServerFixed(name, connection, self)
    self.client = HubClient(name, connection)
    self.error = EventHook()


class SignalrConnectionFixed(signalr.Connection):

  def register_hub(self, name):
    if name not in self._Connection__hubs:
      if self.started:
        raise RuntimeError(
            'Cannot create new hub because connection is already started.')

      self._Connection__hubs[name] = HubFixed(name, self)
    return self._Connection__hubs[name]


class SynchronousSignalrConnection(SignalrConnectionFixed):

  @property
  def send_counter(self):
    return self._Connection__send_counter

  def start(self):
    self.starting.fire()

    negotiate_data = self._Connection__transport.negotiate()
    self.token = negotiate_data['ConnectionToken']

    self._Connection__listener_thread = NullThread()
    listener = self._Connection__transport.start()

    self.is_open = True
    self.started = True

    self.on_open()

    while self.is_open:
      try:
        listener()
      except:
        self.is_open = False
        raise
