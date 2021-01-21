//this var comes from the vertex shader
//they are baricentric interpolated by pixel according to the distance to every vertex
varying vec3 v_position;
varying vec3 v_normal;
varying vec3 v_world_position; //pixel position
varying vec2 v_uv; // texture coordinates

uniform mat4 u_model; //matriz de transformación de local a global

//camera
uniform vec3 u_camera_position;

//textures
uniform sampler3D u_texture; //volume texture
uniform sampler2D jittering; //jittering map
uniform sampler2D lut; //lut map

//Step Length
uniform float step_Length;

//Sliders for manual transfer function
uniform float sl1;
uniform float sl2;

//ImGui
uniform bool jitBool;
uniform bool part1;
uniform bool part2;
uniform bool part2_text;
uniform bool part2_sliders;
uniform bool gradient_option;

//Light
uniform vec3 light_pos;
uniform vec3 light_color;
uniform float light_intensity;

//Gradient
uniform float th; //threshold
uniform float h; //h value

//clipping plane
uniform vec4 clipping;


//Función para calcular el gradiente en un punto del volumen
vec3 gradient(sampler3D texture, vec3 texturePos, float h){
	float x = texture3D(texture, texturePos + vec3(h, 0.0, 0.0)).x - texture3D(texture, texturePos - vec3(h, 0.0, 0.0)).x;
	float y = texture3D(texture, texturePos + vec3(0.0, h, 0.0)).x - texture3D(texture, texturePos - vec3(0.0, h, 0.0)).x;	
	float z = texture3D(texture, texturePos + vec3(0.0, 0.0, h)).x - texture3D(texture, texturePos - vec3(0.0, 0.0, h)).x;
	vec3 gradient = vec3(x,y,z)*(2.0*h);
	return gradient;
}


void main()
{	
	float stepLength = step_Length;
	vec4 finalColor = vec4(0.0);
	vec4 sampleColor = vec4(0.0);

	mat3 u_model_inv = inverse(mat3(u_model)); //inversa de la matrix model (para pasar de global a local)

	vec3 texturePos = (v_position + 1.0)/2.0; //pixel position en coordenadas de textura

	//Camera
	vec3 cameraPosLoc = u_camera_position * u_model_inv; //camera position en coordenadas locales
	vec3 cameraPosText = (cameraPosLoc + 1.0)/2.0; //camera position en coordenadas de textura
	
	//Ray
	vec3 ray = normalize(texturePos - cameraPosText); //ray in texture coordinates
	vec3 stepVector = ray*stepLength;

	//Jittering
	float jit = texture2D(jittering, texturePos.xy).x*stepLength; //jittering value
	if (jitBool==true){
		texturePos += jit*stepVector; //añadimos jittering a la posicion inicial
	}
	
	
	
	//Light
	vec3 lightPosLoc = light_pos * u_model_inv; //light position en coordenadas locales
	vec3 lightPosText = (lightPosLoc + 1.0)/2.0; //light position en coordenadas de textura
	vec3 L = normalize(lightPosText - texturePos); // vector light en coordenadas de textura

	for (int i = 0; i < 1000; i++)
	{ 
		float d = texture3D(u_texture, texturePos).x; //density in the current step
		
		//----------------------------------------------PARTE 1----------------------------------------------
			if(part1 == true){ //ImGui

				sampleColor = vec4(d,d,d,d);
				finalColor += stepLength*(1 - finalColor.a) * sampleColor;
				texturePos += stepVector;

			}

		//----------------------------------------------PARTE 2----------------------------------------------
			if(part2 == true){ //ImGui
				
				if(dot(clipping,vec4(texturePos,1.0)) < 0){ //clipping plane

					//----------------------------------Lut----------------------------------
						//LUT using texture
							if(part2_text == true){ //ImGui

								sampleColor = vec4(texture2D(lut, vec2(d,1)));  //Cargamos valor de el mapa LUT en funcion de la densidad del punto

							}
						//LUT using slicers
							if(part2_sliders == true){ //ImGui

								if (d<sl1) sampleColor=vec4(1.0,0.0,0.0,d); //Red for flesh
									else if (d<sl2) sampleColor=vec4(0.0,1.0,0.0,d); //Green for organs
										else sampleColor = vec4(1.0,1.0,1.0,1.0); //White for bones

							}

						sampleColor.rgb *= sampleColor.a;
						finalColor += stepLength*(1 - finalColor.a) * sampleColor;
			
					//-----------------------Isosusrface and gradients-----------------------
						if(gradient_option == true){ //ImGui

							if (d>=th){

								vec3 N = normalize(gradient(u_texture, texturePos, h)); //Calculamos vector normal
								float NdotL = (dot(N,L) + 1.0)/2.0;//clamp(dot(N,L), 0.00000000001, 0.99999999999); 
								sampleColor = vec4(NdotL)*vec4(light_intensity)*vec4(light_color,1); //Al color final le sumamos el NdotL multiplicado por un color y una intensidad
								finalColor += sampleColor * (1 - finalColor.a);
								finalColor.a = 1; //salimos del for

							} 

						}

				}

				texturePos += stepVector; //next step

			}

			//Condiciones para salir del for
				if (finalColor.a>=1.0) break; //alcanzamos la maxima densidad
				if (texturePos.x<=-0.0 || texturePos.y<=-0.0 || texturePos.z<=-0.0) break; //nos salimos del volumen
				if (jitBool==true){
					if (texturePos.x>=1.0 || texturePos.y>=1.0 || texturePos.z-stepLength>=1.0) break; //si hay jittering los "slices" están desplazados
				}
				else{
					if (texturePos.x>=1.0 || texturePos.y>=1.0 || texturePos.z>=1.0) break; //nos salimos del volumen
				}				

	}

	gl_FragColor = finalColor;

}