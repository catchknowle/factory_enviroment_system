import os
import chardet

def convert_to_utf8(root_dir):
    """
    将目录下所有非UTF-8编码的文件转换为UTF-8编码
    """
    # 遍历目录及其子目录
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            file_path = os.path.join(root, file)
            
            # 跳过二进制文件
            if is_binary_file(file_path):
                continue
            
            try:
                # 检测文件编码
                with open(file_path, 'rb') as f:
                    raw_data = f.read()
                    
                # 如果文件为空，跳过
                if not raw_data:
                    continue
                
                # 检测编码
                result = chardet.detect(raw_data)
                encoding = result['encoding']
                confidence = result['confidence']
                
                # 如果编码是UTF-8或检测不到编码，跳过
                if encoding == 'utf-8' or encoding is None:
                    continue
                
                # 打印转换信息
                print(f"转换文件: {file_path}")
                print(f"原始编码: {encoding} (置信度: {confidence:.2f})")
                
                # 以原始编码读取文件
                try:
                    with open(file_path, 'r', encoding=encoding) as f:
                        content = f.read()
                    
                    # 以UTF-8编码写入文件
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                    
                    print(f"转换成功: {file_path} -> UTF-8")
                except Exception as e:
                    print(f"转换失败: {file_path}, 错误: {e}")
                    
            except Exception as e:
                print(f"处理文件失败: {file_path}, 错误: {e}")

def is_binary_file(file_path):
    """
    判断文件是否为二进制文件
    """
    try:
        with open(file_path, 'rb') as f:
            chunk = f.read(1024)
            
        # 检查是否包含NULL字节
        if b'\x00' in chunk:
            return True
            
        # 检查非文本字符的比例
        text_chars = bytearray({7, 8, 9, 10, 12, 13, 27} | set(range(0x20, 0x100)))
        non_text = sum(1 for byte in chunk if byte not in text_chars)
        return non_text / len(chunk) > 0.3
        
    except Exception:
        return True

if __name__ == "__main__":
    # 转换当前目录
    current_dir = os.getcwd()
    print(f"开始转换目录: {current_dir}")
    convert_to_utf8(current_dir)
    print("转换完成！")