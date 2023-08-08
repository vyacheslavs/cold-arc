DROP TABLE IF EXISTS db_settings;

CREATE TABLE IF NOT EXISTS db_settings (
    version       INTEGER NOT NULL,
    name          TEXT NOT NULL CHECK( LENGTH(name) <= 100 ),
    current_media INTEGER
);

INSERT INTO db_settings (version, name) VALUES (6, "my archive");

DROP TABLE IF EXISTS arc_tree;
CREATE TABLE IF NOT EXISTS arc_tree(
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER,
    typ       TEXT NOT NULL CHECK( typ in ('folder', 'file') ),
    name      TEXT NOT NULL,
    hash      TEXT,
    lnk       TEXT,
    dt        INTEGER,
    UNIQUE(name, parent_id)
);

DROP TABLE IF EXISTS arc_media;
CREATE TABLE IF NOT EXISTS arc_media (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    capacity  INTEGER NOT NULL,
    occupied  INTEGER NOT NULL,
    locked    INTEGER NOT NULL,
    name      TEXT NOT NULL,
    serial    TEXT NOT NULL UNIQUE
);

DROP TABLE IF EXISTS arc_tree_to_media;
CREATE TABLE IF NOT EXISTS arc_tree_to_media (
    arc_tree_id  INTEGER NOT NULL,
    arc_media_id INTEGER NOT NULL,
    UNIQUE(arc_tree_id, arc_media_id)
);
