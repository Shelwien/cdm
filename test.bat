@echo off

bwth c A10.jpg A10.bwth

cdm c A10.bwth A10.bwth.cdm

cdm d A10.bwth.cdm A10.bwth.unp

bwth d A10.bwth.unp A10.unp

fc /b A10.jpg A10.unp

