-- MySQL dump 10.13  Distrib 8.0.18, for Win64 (x86_64)
--
-- Host: 127.0.0.1    Database: flight_ticket
-- ------------------------------------------------------
-- Server version	8.0.18

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `flight`
--

DROP TABLE IF EXISTS `flight`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `flight` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `flight_no` varchar(16) COLLATE utf8mb4_unicode_ci NOT NULL,
  `from_city` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  `to_city` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  `depart_time` datetime NOT NULL,
  `arrive_time` datetime NOT NULL,
  `price_cents` int(11) NOT NULL,
  `seat_total` int(11) NOT NULL,
  `seat_left` int(11) NOT NULL,
  `status` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `flight_no` (`flight_no`),
  UNIQUE KEY `uk_flight_no` (`flight_no`),
  KEY `idx_route_time` (`from_city`,`to_city`,`depart_time`),
  KEY `idx_flight_no` (`flight_no`),
  CONSTRAINT `ck_flight_seat` CHECK (((`seat_total` >= 0) and (`seat_left` >= 0) and (`seat_left` <= `seat_total`))),
  CONSTRAINT `ck_flight_status` CHECK ((`status` in (0,1,2))),
  CONSTRAINT `ck_flight_time` CHECK ((`arrive_time` > `depart_time`))
) ENGINE=InnoDB AUTO_INCREMENT=18 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flight`
--

LOCK TABLES `flight` WRITE;
/*!40000 ALTER TABLE `flight` DISABLE KEYS */;
INSERT INTO `flight` VALUES (1,'CA1234','北京','上海','2025-12-20 10:30:00','2025-12-20 12:45:00',56000,180,42,0),(2,'MU8888','上海','广州','2025-12-21 09:10:00','2025-12-21 11:40:00',72000,160,80,0),(3,'CA1501','北京','上海','2025-12-20 08:00:00','2025-12-20 10:30:00',128000,180,179,0),(4,'CA1502','上海','北京','2025-12-20 14:00:00','2025-12-20 16:30:00',119000,180,180,0),(5,'CZ3101','北京','广州','2025-12-20 09:15:00','2025-12-20 12:45:00',156000,200,200,0),(6,'CZ3102','广州','北京','2025-12-20 15:00:00','2025-12-20 18:30:00',148000,200,200,0),(7,'ZH9101','深圳','北京','2025-12-20 07:30:00','2025-12-20 11:00:00',168000,160,160,0),(8,'ZH9102','北京','深圳','2025-12-20 13:00:00','2025-12-20 16:30:00',159000,160,160,0),(9,'MU5401','成都','上海','2025-12-20 10:00:00','2025-12-20 13:20:00',135000,190,190,0),(10,'MU5402','上海','成都','2025-12-20 16:00:00','2025-12-20 19:20:00',128000,190,190,0),(11,'MF8501','杭州','北京','2025-12-20 08:45:00','2025-12-20 11:15:00',108000,150,150,0),(12,'MF8502','北京','杭州','2025-12-20 14:30:00','2025-12-20 17:00:00',99000,150,150,0),(13,'3U8701','重庆','广州','2025-12-20 09:30:00','2025-12-20 11:50:00',89000,170,170,0),(14,'3U8702','广州','重庆','2025-12-20 15:30:00','2025-12-20 17:50:00',85000,170,170,0),(15,'HU7101','西安','北京','2025-12-20 07:15:00','2025-12-20 09:45:00',78000,140,140,0),(16,'HU7102','北京','西安','2025-12-20 12:15:00','2025-12-20 14:45:00',75000,140,140,0),(17,'CZ6501','武汉','深圳','2025-12-20 11:00:00','2025-12-20 13:30:00',95000,180,180,0);
/*!40000 ALTER TABLE `flight` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `orders`
--

DROP TABLE IF EXISTS `orders`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `orders` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) DEFAULT NULL,
  `flight_id` bigint(20) DEFAULT NULL,
  `passenger_name` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `passenger_id_card` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `price_cents` int(11) NOT NULL,
  `status` tinyint(4) NOT NULL DEFAULT '0',
  `created_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `seat_num` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_orders_user_time` (`user_id`,`created_time`),
  KEY `idx_orders_flight` (`flight_id`),
  CONSTRAINT `fk_order_flight` FOREIGN KEY (`flight_id`) REFERENCES `flight` (`id`) ON DELETE SET NULL,
  CONSTRAINT `fk_order_user` FOREIGN KEY (`user_id`) REFERENCES `user` (`id`) ON DELETE SET NULL,
  CONSTRAINT `ck_order_price` CHECK ((`price_cents` > 0)),
  CONSTRAINT `ck_order_status` CHECK ((`status` in (0,1,2,3)))
) ENGINE=InnoDB AUTO_INCREMENT=25 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `orders`
--

LOCK TABLES `orders` WRITE;
/*!40000 ALTER TABLE `orders` DISABLE KEYS */;
INSERT INTO `orders` VALUES (1,1,1,'张三','110101199003077890',128000,1,'2025-12-19 09:15:23','12'),(2,2,2,'李四','310101198905124567',119000,1,'2025-12-19 10:20:45','8'),(3,3,3,'王五','440101199508238765',156000,1,'2025-12-19 11:05:12','18'),(4,4,4,'赵六','510101199210159876',148000,2,'2025-12-19 14:30:56','22'),(5,5,5,'孙七','500101200001011234',168000,1,'2025-12-19 08:40:33','9'),(6,6,6,'周八','330101198807185678',159000,1,'2025-12-19 15:10:18','15'),(7,7,7,'司马九','430101199309206789',135000,3,'2025-12-19 12:25:49','3'),(8,8,8,'陈十','320101199104257890',128000,1,'2025-12-19 16:45:07','28'),(9,9,9,'刘十一','610101198712108901',108000,1,'2025-12-19 07:50:21','11'),(10,10,10,'王十二','120101199406309012',99000,1,'2025-12-19 13:15:37','7'),(11,1,11,'张三','110101199003077890',89000,1,'2025-12-19 09:55:42','19'),(12,2,12,'李四','310101198905124567',85000,2,'2025-12-19 17:05:14','25'),(13,3,13,'王五','440101199508238765',78000,1,'2025-12-19 06:30:58','5'),(14,4,14,'赵六','510101199210159876',75000,1,'2025-12-19 11:40:09','17'),(15,5,15,'孙七','500101200001011234',95000,1,'2025-12-19 10:05:31','21'),(16,6,1,'周八','330101198807185678',128000,1,'2025-12-19 14:20:47','14'),(17,7,2,'司马九','430101199309206789',119000,3,'2025-12-19 15:50:16','8'),(18,8,3,'陈十','320101199104257890',156000,1,'2025-12-19 08:15:29','30'),(19,9,4,'刘十一','610101198712108901',148000,1,'2025-12-19 12:50:03','10'),(20,10,5,'王十二','120101199406309012',168000,2,'2025-12-19 09:30:51','27'),(21,1,3,'1','1',128000,1,'2025-12-23 12:07:10','2'),(22,1,3,'2','2',128000,1,'2025-12-23 12:08:55','3'),(23,1,3,'ccc','12345',128000,1,'2025-12-23 12:12:36','4'),(24,1,3,'和的萨芬科技股份','123456789012345678',128000,0,'2025-12-24 19:10:12','2');
/*!40000 ALTER TABLE `orders` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `passenger`
--

DROP TABLE IF EXISTS `passenger`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `passenger` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '乘机人ID',
  `user_id` bigint(20) NOT NULL COMMENT '所属用户ID（关联user表主键）',
  `name` varchar(50) NOT NULL COMMENT '乘机人姓名',
  `id_card` varchar(18) NOT NULL COMMENT '身份证号',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_user_id_id_card` (`user_id`,`id_card`) COMMENT '同一用户不能添加相同身份证的乘机人'
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='乘机人信息表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `passenger`
--

LOCK TABLES `passenger` WRITE;
/*!40000 ALTER TABLE `passenger` DISABLE KEYS */;
INSERT INTO `passenger` VALUES (4,1,'陈','123456789012345678');
/*!40000 ALTER TABLE `passenger` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user`
--

DROP TABLE IF EXISTS `user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `user` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `username` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `password` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `phone` varchar(20) COLLATE utf8mb4_unicode_ci NOT NULL,
  `real_name` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  `id_card` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`),
  UNIQUE KEY `phone` (`phone`),
  UNIQUE KEY `id_card` (`id_card`)
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `user`
--

LOCK TABLES `user` WRITE;
/*!40000 ALTER TABLE `user` DISABLE KEYS */;
INSERT INTO `user` VALUES (1,'1','1','15278400113','张三','110101199003077890'),(2,'2','2','13900139001','李四','310101198905124567'),(3,'wangwu777','777777Cc','13700137002','王五','440101199508238765'),(4,'zhaoliu666','666666Dd','13600136003','赵六','510101199210159876'),(5,'sunqi999','999999Ee','13500135004','孙七','500101200001011234'),(6,'zhouba888','888888Ff','13400134005','周八','330101198807185678'),(7,'wuyan12345','555555Gg','13300133006','司马九','430101199309206789'),(8,'chenshi001','000000Hh','13200132007','陈十','320101199104257890'),(9,'liushi002','111111Ii','19900199008','刘十一','610101198712108901'),(10,'wangshi003','222222Jj','19800198009','王十二','120101199406309012');
/*!40000 ALTER TABLE `user` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2025-12-26 17:02:12
