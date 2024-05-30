USE master;
GO

DECLARE @DatabaseName NVARCHAR(100);
SET @DatabaseName = '$(DatabaseName)';

DECLARE @DataFileName SYSNAME, @LogFileName SYSNAME;
SET @DataFileName = @DatabaseName + '_dat.mdf';
SET @LogFileName  = @DatabaseName + '_log.ldf';

DECLARE @DefaultDataPath SYSNAME, @DefaultLogPath SYSNAME;
SET @DefaultDataPath = CONCAT(CONVERT(SYSNAME, SERVERPROPERTY('InstanceDefaultDataPath')), @DataFileName);
SET @DefaultLogPath = CONCAT(CONVERT(SYSNAME, SERVERPROPERTY('InstanceDefaultLogPath')), @LogFileName);

DECLARE @SQLString NVARCHAR(500);

SET @SQLString = FORMATMESSAGE(N'
CREATE DATABASE %s ON
(NAME = mmocraft_dat,
    FILENAME = "%s",
    SIZE = 100 MB,
    FILEGROWTH = 100 MB)
LOG ON
(NAME = mmocraft_log,
    FILENAME = "%s",
    SIZE = 100 MB,
    FILEGROWTH = 100 MB);', @DatabaseName, @DefaultDataPath, @DefaultLogPath);

EXECUTE SP_EXECUTESQL @SQLString
GO