# High performing AI for Face Detection for T31 chipset. 


xa_sdk_initialize() 

 I would add xa_sdk_configure()  with the config blob in the Json HOWTO - this will enable face recognition.  A call to  xa_sdk_is_face_recognition_enabled() should return the correct values as specified in the .h file.  Then add a call to  xa_sdk_process_image() with an image with a face you'd like to test - call that function over and over with the same image until it returns some data in process_image_outputs (edited) 


#to compile 

cd MagicAI

./build.sh

cd /src/facedetec

make -j8



# to test
copy binary runAI to t31 at /mnt

cd /mnt 
runConfTest 
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt


