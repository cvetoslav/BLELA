import asyncio
from bleak import BleakScanner, BleakClient, BleakGATTCharacteristic


SERV_LA = "6a09fdbd-664d-4e00-8e92-d9e51c6eba93"
CHAR_SETUP = "5a61383f-e7e0-4e92-bc52-4ff803be0f1c"
CHAR_START = "3642a423-afd4-4ab1-95b2-5f7bff961610"
CHAR_DATA = "ea91a314-ab37-4f3b-8a58-d3660b5ab81a"
CHAR_READY = "d80ca4bd-7c02-4728-bf8a-3e6c10c4b378"

dat = []

def ready_cb(sender: BleakGATTCharacteristic, data: bytearray):
    if data[0] == 0:
        print("Start sampling")
    else:
        print("Data received")
        print(dat)


def data_cb(sender: BleakGATTCharacteristic, data: bytearray):
    for i in range(10):
        dat.append((int.from_bytes(data[2*i:2*i+2], 'little') & 0b100) >> 2)
        #print(bin(int.from_bytes(data[2*i:2*i+2], 'little') & 0b100)[2:])


async def scan(name='BLELA'):
    devices = await BleakScanner.discover()
    ret = []
    for d in devices:
        if d.name == name:
            ret.append(d.address)
    return ret


async def main():
    devs = ['E4:8D:8E:85:6B:70']
    while len(devs) == 0:
        devs = await scan()

    print(devs)

    async with BleakClient(devs[0]) as client:
        # await client.connect()
        await client.start_notify(CHAR_DATA, data_cb)
        print("connected")

        sample_freq = 40000
        num_samples = 8000

        await client.write_gatt_char(CHAR_SETUP, sample_freq.to_bytes(4, 'little'))
        await asyncio.sleep(0.2)
        resp = await client.read_gatt_char(CHAR_SETUP)
        resp = int.from_bytes(resp, 'little')
        if resp != sample_freq:
            print(f"Error while setting up sampling frequency! Wrote: {sample_freq}, read: {resp}")
        else:
            await client.start_notify(CHAR_READY, ready_cb)
            await client.write_gatt_char(CHAR_START, num_samples.to_bytes(4, 'little'))

        while True:
            await asyncio.sleep(10)

asyncio.run(main())
