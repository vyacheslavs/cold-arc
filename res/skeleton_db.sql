DROP TABLE IF EXISTS db_settings;

CREATE TABLE IF NOT EXISTS db_settings (
    version INTEGER NOT NULL,
    name    TEXT NOT NULL CHECK( LENGTH(name) <= 100 )
);

INSERT INTO db_settings (version, name) VALUES (1, "my archive");

DROP TABLE IF EXISTS arc_tree;
CREATE TABLE IF NOT EXISTS arc_tree(
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER,
    typ       TEXT NOT NULL CHECK( typ in ('folder', 'file') ),
    media_id  INTEGER NOT NULL,
    hash      TEXT NOT NULL
);

DROP TABLE IF EXISTS arc_media;
CREATE TABLE IF NOT EXISTS arc_media (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    capacity  INTEGER NOT NULL,
    name      TEXT NOT NULL,
    serial    TEXT NOT NULL
);