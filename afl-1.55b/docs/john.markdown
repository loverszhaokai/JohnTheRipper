# John input file format

## XML Schema

```
<?xml version="1.0" encoding="utf-8"?>
<xs:schema>

	<xs:element name="AFL">
		<xs:annotation>
			<xs:documentation>Root element of a John XML DDL document.
			</xs:documentation>
		</xs:annotation>
		<xs:complexType>
			<xs:choice minOccurs="1" maxOccurs="unbounded">
				   <xs:element ref="DataElement" />
			</xs:choice>
			<xs:attribute name="case_number">
				   <xs:annotation>
						<xs:documentation>The total number of mutated cases. It is must be an integer. Default is 1000.
						</xs:documentation>
				   </xs:annotation>
			</xs:attribute>
		</xs:complexType>
	</xs:element>

	<xs:element name="DataElement">
		<xs:annotation>
			<xs:documentation>It is a container and it contains other elements including DataElement.
			</xs:documentation>
                </xs:annotation>
	</xs:element>

	<xs:element name="string">
		<xs:annotation>
			<xs:documentation>A string of characters.
	  		</xs:documentation>
		</xs:annotation>
		<xs:complexType>
			<xs:attribute name="type">
				   <xs:annotation>
						<xs:documentation>Specify type of string. Default is STR.
						</xs:documentation>
				   </xs:annotation>
				   <xs:simpleType>
						<xs:restriction base="xs:string">
								<xs:enumeration value="STR">
										<xs:annotation>
											<xs:documentation>String of characters
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="NUM">
										<xs:annotation>
											<xs:documentation>String of characters which is from '0' to '9'
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="HEX_STR">
										<xs:annotation>
											<xs:documentation>String of hex characters, contains botn upper case and lower case
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="HEX_STR_U">
										<xs:annotation>
											<xs:documentation>String of hex characters, contains only upper case
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="HEX_STR_L">
										<xs:annotation>
											<xs:documentation>String of hex characters, contains only lower case
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
						</xs:restriction>
				   </xs:simpleType>
		        </xs:attribute>
			<xs:attribute name="length">
				   <xs:annotation>
						<xs:documentation>The length of the characters. length="40": the length must be 40; length="[min,max]": the length is from min to max; length="[,max]" the length is from 0 to max; length="[min,]": the length is from min to STRING_MAX_SIZE
						</xs:documentation>
				   </xs:annotation>
			</xs:attribute>
			<xs:attribute name="is_mutate">
				   <xs:annotation>
						<xs:documentation>Is current element should be mutated
						</xs:documentation>
				   </xs:annotation>
				   <xs:simpleType>
						<xs:restriction base="xs:string">
								<xs:enumeration value="true">
										<xs:annotation>
											<xs:documentation>Default value, this element can be mutated
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="false">
										<xs:annotation>
											<xs:documentation>Never mutate this element
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
						</xs:restriction>
				   </xs:simpleType>
			</xs:attribute>
		</xs:complexType>
	</xs:element>	

</xs:schema>
```


## Samples


### siemens-s7.xml(valid)


```
<?xml version="1.0" encoding="UTF-8"?>
<AFL case_number="90">

<!-- Valid format -->
<DataElement>
  <string is_mutate="false">$siemens-s7</string>                                           <!--id=1-->
  <string is_mutate="false">$</string>                                                     <!--id=2-->
  <string is_mutate="false" length="1" type="NUM">1</string>                               <!--id=3-->
  <string is_mutate="false">$</string>                                                     <!--id=4-->
  <string length="40" type="HEX_STR_L">599fe00cdb61f76cc6e949162f22c95943468acb</string>   <!--id=5-->
  <string is_mutate="false">$</string>                                                     <!--id=6-->
  <string length="40" type="HEX_STR_L">002e45951f62602b2f5d15df217f49da2f5379cb</string>   <!--id=7-->
</DataElement>

</AFL>

```

AFL will generate **90** cases. And **all** of those cases **can** pass the valid() of src/siemens-s7_fmt_plug.c.
Because only the No.5 and No.7 elements can be mutated, and they can only be replaced by those string whose length is 40 and which are lower hex string.


### siemens-s7.xml(Invalid)


```
<?xml version="1.0" encoding="UTF-8"?>
<AFL case_number="90">

<!-- Invalid format -->
<DataElement>
  <string>$siemens-s7</string>                                                             <!--id=1-->
  <string>$</string>                                                                       <!--id=2-->
  <stringtype="NUM">1</string>                                                             <!--id=3-->
  <string>$</string>                                                                       <!--id=4-->
  <string type="HEX_STR_L">599fe00cdb61f76cc6e949162f22c95943468acb</string>               <!--id=5-->
  <string>$</string>                                                                       <!--id=6-->
  <string length="40" type="HEX_STR_U">002e45951f62602b2f5d15df217f49da2f5379cb</string>   <!--id=7-->
</DataElement>

</AFL>

```

AFL will generate 90 cases. And **most** of those cases **can not** pass the valid() of src/siemens-s7_fmt_plug.c.
Because each elements can be mutated. And for example once the No.1 element is mutated, the case will not pass the valid().


## Fuzz Algorithm

There are N elements which should be mutated. In each iteration, AFL will randomly generates an integer: mutate_element_num which is from 1 to N. Then, randomly select mutate_element_num elements to mutate. Finally, generates a case.
The Fuzz Algorithm is as follow:

```

int mutate_elements_num = rand_int(1, N + 1);	// rand_int(min, max) will randomly return a int from 
    			  	      	      	// min to max, including min and excluding max

int elements_id[] = { 1, 2, ..., N };

for (i = 0; i < mutate_elements_num; ++i)
    int r = rand_int(i, N);
    int id = elements_id[r];
    elements_id[r] = elements_id[i];

    mutate the id th elements

generate a case

```

## Built-in functions


### type=STR


// Randomly return a string of characters which length is from min to max
rand_str(min, max);


### type=NUM


// Randomly return a number string which length is from min to max
rand_num_str(min, max);


### type=HEX_STR


// Randomly return a hex string which length is from min to max, contains both upper case and lower case
rand_hex_str(min, max);


### type=HEX_STR_U


// Randomly return a hex string which length is from min to max, contains only upper case
rand_hex_str_u(min, max);


### type=HEX_STR_L


// Randomly return a hex string which length is from min to max, contains only lower case
rand_hex_str_l(min, max);



