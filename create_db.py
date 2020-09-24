from sqlite3 import *

conn = connect('items.db')

create_users_table = """CREATE TABLE IF NOT EXISTS users(
    id integer PRIMARY KEY,
    username text NOT NULL,
    password text NOT NULL
);"""

create_items_table = """CREATE TABLE IF NOT EXISTS items(
    id integer PRIMARY KEY,
    name text NOT NULL,
    armorPoints integer NOT NULL,
    healthPoints integer NOT NULL,
    manaPoints integer NOT NULL,
    sellPrice integer NOT NULL,
    damage integer NOT NULL,
    critChance real NOT NULL,
    range integer NOT NULL
);"""

c = conn.cursor()

c.execute("DROP TABLE IF EXISTS users;")
c.execute(create_users_table)
c.execute("DROP TABLE IF EXISTS items;")
c.execute(create_items_table)

sql = "INSERT INTO items(name, armorPoints, healthPoints, manaPoints, sellPrice, damage, critChance, range) VALUES (?,?,?,?,?,?,?,?)"

with open('items.csv') as f:
    for line in f:
        c.execute(sql, line.strip().split(','))

conn.commit()
conn.close()