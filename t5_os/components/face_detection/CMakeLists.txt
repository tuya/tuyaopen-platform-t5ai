set(incs)
set(srcs)

list(APPEND incs
	include
	)
list(APPEND srcs
	facedetectcnn-data.c
	facedetectcnn-model.c
	facedetectcnn.c
	)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
