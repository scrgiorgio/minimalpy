
# ExampleClass
class ExampleClass:
  """
  doc line 1
  doc line 2
  """
  
  static_value = 12345
  
  def __init__(self):
    self.value="abcde"



# main
def main():
  example=ExampleClass()
  print(ExampleClass.static_value)
  print(example.value)

# entry point
if __name__ == '__main__':
  main()
